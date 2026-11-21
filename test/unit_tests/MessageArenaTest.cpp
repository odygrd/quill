#include "DocTestExtensions.h"
#include "doctest/doctest.h"

#include "quill/backend/MessageArena.h"

#include "quill/bundled/fmt/format.h"

#include <cstddef>
#include <deque>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

TEST_SUITE_BEGIN("MessageArena");

using namespace quill::detail;

namespace
{
/** Writes a message to the arena and commits it */
std::string_view add_message(MessageArena& arena, std::string_view message)
{
  fmtquill::detail::buffer<char>& buffer = arena.begin_message();
  buffer.append(message.data(), message.data() + message.size());
  return arena.commit_message();
}
} // namespace

/***/
TEST_CASE("message_arena_keeps_messages_back_to_back_in_order")
{
  MessageArena arena{256};
  REQUIRE(arena.empty());

  std::string_view const first = add_message(arena, "first message");
  std::string_view const second = add_message(arena, "second");
  std::string_view const third = add_message(arena, "the third message");

  REQUIRE_FALSE(arena.empty());
  REQUIRE_EQ(first, "first message");
  REQUIRE_EQ(second, "second");
  REQUIRE_EQ(third, "the third message");

  // Each message starts where the previous one ends
  REQUIRE_EQ(second.data(), first.data() + first.size());
  REQUIRE_EQ(third.data(), second.data() + second.size());

  // Releasing the oldest message leaves the others where they are
  arena.pop_front();
  REQUIRE_FALSE(arena.empty());
  REQUIRE_EQ(second, "second");
  REQUIRE_EQ(third, "the third message");

  arena.pop_front();
  arena.pop_front();
  REQUIRE(arena.empty());
}

/***/
TEST_CASE("message_arena_formats_a_message_in_place")
{
  MessageArena arena{256};

  std::string_view const before = add_message(arena, "before");

  // fmt writes the message, in several pieces, straight after the previous one
  fmtquill::detail::buffer<char>& buffer = arena.begin_message();
  REQUIRE_EQ(buffer.size(), 0);

  fmtquill::format_to(fmtquill::appender{buffer}, "order {} filled at {:.2f}", 42, 101.5);
  fmtquill::format_to(fmtquill::appender{buffer}, " ({}x)", 3);
  REQUIRE_EQ(&arena.message(), &buffer);

  std::string_view const formatted = arena.commit_message();
  REQUIRE_EQ(formatted, "order 42 filled at 101.50 (3x)");
  REQUIRE_EQ(formatted.data(), before.data() + before.size());
  REQUIRE_EQ(before, "before");
}

/***/
TEST_CASE("message_arena_drops_a_message_that_is_not_committed")
{
  MessageArena arena{256};

  std::string_view const first = add_message(arena, "first");

  // An event that is not added to the buffer leaves its message behind
  fmtquill::detail::buffer<char>& abandoned = arena.begin_message();
  std::string_view const abandoned_text{"abandoned message"};
  abandoned.append(abandoned_text.data(), abandoned_text.data() + abandoned_text.size());

  std::string_view const second = add_message(arena, "second");
  REQUIRE_EQ(second, "second");
  REQUIRE_EQ(second.data(), first.data() + first.size());

  // Only the two committed messages are released
  arena.pop_front();
  REQUIRE_FALSE(arena.empty());
  arena.pop_front();
  REQUIRE(arena.empty());
}

/***/
TEST_CASE("message_arena_stores_empty_messages")
{
  MessageArena arena{256};

  // Nothing has been written yet, so an empty message needs no memory
  std::string_view const no_memory = add_message(arena, "");
  REQUIRE(no_memory.empty());
  REQUIRE_FALSE(arena.empty());
  REQUIRE_EQ(arena.allocated_bytes(), 0);

  std::string_view const first = add_message(arena, "first");
  std::string_view const empty = add_message(arena, "");
  std::string_view const second = add_message(arena, "second");

  REQUIRE_EQ(first, "first");
  REQUIRE(empty.empty());
  REQUIRE_EQ(second, "second");
  REQUIRE_EQ(second.data(), first.data() + first.size());

  // A second commit without a begin_message() is an empty message, not the previous one again
  std::string_view const committed_twice = arena.commit_message();
  REQUIRE(committed_twice.empty());
  REQUIRE_EQ(second, "second");

  // Each message is released on its own, the empty ones included
  for (size_t i = 0; i < 5; ++i)
  {
    REQUIRE_FALSE(arena.empty());
    arena.pop_front();
  }

  REQUIRE(arena.empty());
}

/***/
TEST_CASE("message_arena_writes_an_empty_block_from_its_start")
{
  MessageArena arena{256};

  std::string_view const first = add_message(arena, "a message");
  char const* const block_start = first.data();
  REQUIRE_EQ(arena.allocated_bytes(), 256);

  // While each message is released before the next one, they all use the same memory
  for (size_t i = 0; i < 1000; ++i)
  {
    arena.pop_front();
    REQUIRE(arena.empty());

    std::string_view const next = add_message(arena, "a message that follows");
    REQUIRE_EQ(next.data(), block_start);
    REQUIRE_EQ(next, "a message that follows");
  }

  REQUIRE_EQ(arena.allocated_bytes(), 256);
}

/***/
TEST_CASE("message_arena_continues_in_the_next_block")
{
  constexpr size_t block_capacity{64};
  MessageArena arena{block_capacity};

  // Three messages of 20 bytes fit a block of 64, so ten of them take four blocks
  std::vector<std::string> expected;
  std::vector<std::string_view> stored;

  for (size_t i = 0; i < 10; ++i)
  {
    expected.push_back(std::string(20, static_cast<char>('a' + i)));
    stored.push_back(add_message(arena, expected.back()));
  }

  REQUIRE_EQ(arena.allocated_bytes(), 4 * block_capacity);

  for (size_t i = 0; i < 10; ++i)
  {
    REQUIRE_EQ(stored[i], expected[i]);
  }

  // The fourth message did not fit the first block and starts another one
  REQUIRE_EQ(stored[1].data(), stored[0].data() + 20);
  REQUIRE_EQ(stored[2].data(), stored[1].data() + 20);
  REQUIRE_NE(stored[3].data(), stored[2].data() + 20);
}

/***/
TEST_CASE("message_arena_moves_a_message_that_outgrows_its_block")
{
  constexpr size_t block_capacity{64};
  MessageArena arena{block_capacity};

  std::string const first_text(50, 'x');
  std::string_view const first = add_message(arena, first_text);

  // 10 bytes still fit after the first message, the next 30 do not: the whole message moves to
  // the next block, so it stays in one piece
  fmtquill::detail::buffer<char>& buffer = arena.begin_message();
  std::string const head(10, 'h');
  std::string const tail(30, 't');
  buffer.append(head.data(), head.data() + head.size());
  REQUIRE_EQ(buffer.data(), first.data() + first.size());

  buffer.append(tail.data(), tail.data() + tail.size());
  REQUIRE_NE(buffer.data(), first.data() + first.size());

  std::string_view const second = arena.commit_message();
  REQUIRE_EQ(second, head + tail);
  REQUIRE_EQ(first, first_text);
  REQUIRE_EQ(arena.allocated_bytes(), 2 * block_capacity);
}

/***/
TEST_CASE("message_arena_stores_a_message_longer_than_a_block")
{
  constexpr size_t block_capacity{64};
  MessageArena arena{block_capacity};

  std::string_view const before = add_message(arena, "before");

  // Written in small pieces, so it outgrows its memory several times
  std::string expected;
  fmtquill::detail::buffer<char>& buffer = arena.begin_message();

  for (size_t i = 0; i < 300; ++i)
  {
    std::string const piece = "piece" + std::to_string(i) + ";";
    buffer.append(piece.data(), piece.data() + piece.size());
    expected += piece;
  }

  std::string_view const long_message = arena.commit_message();
  REQUIRE_GT(long_message.size(), 10 * block_capacity);
  REQUIRE_EQ(long_message, expected);
  REQUIRE_EQ(before, "before");
  REQUIRE_GE(arena.allocated_bytes(), block_capacity + expected.size());

  // Messages keep their order around it
  std::string_view const after = add_message(arena, "after");
  REQUIRE_EQ(after, "after");
  REQUIRE_EQ(long_message, expected);

  arena.pop_front();
  arena.pop_front();
  REQUIRE_FALSE(arena.empty());
  REQUIRE_EQ(after, "after");

  arena.pop_front();
  REQUIRE(arena.empty());
}

/***/
TEST_CASE("message_arena_grows_a_long_first_message_in_its_block")
{
  constexpr size_t block_capacity{64};
  MessageArena arena{block_capacity};

  // No message is committed, so the block of this one gets more memory instead of a new block
  std::string const expected(1000, 'z');
  std::string_view const long_message = add_message(arena, expected);
  REQUIRE_EQ(long_message, expected);
  REQUIRE_GE(arena.allocated_bytes(), expected.size());

  // Once it is released its memory is freed, and the block is allocated with the usual size for
  // the messages that follow
  arena.pop_front();
  REQUIRE_EQ(arena.allocated_bytes(), 0);

  std::string_view const next = add_message(arena, "next");
  REQUIRE_EQ(next, "next");
  REQUIRE_EQ(arena.allocated_bytes(), block_capacity);
}

/***/
TEST_CASE("message_arena_reuses_its_blocks_in_a_ring")
{
  constexpr size_t block_capacity{64};
  constexpr size_t backlog{8};
  MessageArena arena{block_capacity};

  // A steady backlog of 8 messages of 20 bytes, three to a block, goes around a few blocks
  std::deque<std::pair<std::string_view, std::string>> buffered;
  size_t allocated_bytes_after_warm_up{0};

  for (size_t i = 0; i < 5000; ++i)
  {
    std::string expected(20, static_cast<char>('a' + (i % 26)));
    std::string_view const stored = add_message(arena, expected);
    buffered.emplace_back(stored, std::move(expected));

    if (buffered.size() > backlog)
    {
      REQUIRE_EQ(buffered.front().first, buffered.front().second);
      buffered.pop_front();
      arena.pop_front();
    }

    if (i == 100)
    {
      allocated_bytes_after_warm_up = arena.allocated_bytes();
    }
  }

  // The memory did not grow with the number of messages
  REQUIRE_EQ(arena.allocated_bytes(), allocated_bytes_after_warm_up);
  REQUIRE_LE(arena.allocated_bytes(), 5 * block_capacity);

  while (!buffered.empty())
  {
    REQUIRE_EQ(buffered.front().first, buffered.front().second);
    buffered.pop_front();
    arena.pop_front();
  }

  REQUIRE(arena.empty());
}

/***/
TEST_CASE("message_arena_matches_a_queue_of_strings")
{
  constexpr size_t block_capacity{128};
  MessageArena arena{block_capacity};

  // Bursts that build a backlog and drain it again, with messages from empty to several blocks
  std::deque<std::pair<std::string_view, std::string>> buffered;
  std::mt19937 rng{20240607};
  std::uniform_int_distribution<size_t> small_size{0, 100};
  std::uniform_int_distribution<size_t> large_size{block_capacity, 4 * block_capacity};
  std::uniform_int_distribution<int> percent{0, 99};

  auto const release_oldest = [&arena, &buffered]()
  {
    REQUIRE_EQ(buffered.front().first, buffered.front().second);
    buffered.pop_front();
    arena.pop_front();
  };

  for (size_t burst = 0; burst < 200; ++burst)
  {
    size_t const messages_added = static_cast<size_t>(percent(rng)) + 1;

    for (size_t i = 0; i < messages_added; ++i)
    {
      size_t const size = (percent(rng) < 5) ? large_size(rng) : small_size(rng);
      std::string expected(size, static_cast<char>('a' + (percent(rng) % 26)));
      std::string_view const stored = add_message(arena, expected);
      REQUIRE_EQ(stored.size(), size);
      buffered.emplace_back(stored, std::move(expected));
    }

    // Every third burst is drained completely, the others partly
    // The percentage is a variable of its own: GCC 8 reports a cast inside the conditional
    // expression below as a sign conversion
    size_t const released_percent = static_cast<size_t>(percent(rng));
    size_t const messages_released =
      ((burst % 3) == 2) ? buffered.size() : (buffered.size() * released_percent / 100);

    for (size_t i = 0; i < messages_released; ++i)
    {
      release_oldest();
    }

    REQUIRE_EQ(arena.empty(), buffered.empty());
  }

  // The messages that are still buffered were not overwritten by the ones that came after them
  while (!buffered.empty())
  {
    release_oldest();
  }

  REQUIRE(arena.empty());
}

/***/
TEST_CASE("message_arena_frees_the_block_of_a_long_message_when_it_is_released")
{
  constexpr size_t block_capacity{64};
  MessageArena arena{block_capacity};

  (void)add_message(arena, "before");

  // A message longer than a block gets a block that fits it
  std::string const long_text(100 * block_capacity, 'L');
  std::string_view const long_message = add_message(arena, long_text);
  REQUIRE_EQ(long_message, long_text);
  REQUIRE_GE(arena.allocated_bytes(), long_text.size());

  std::string_view const after = add_message(arena, "after");

  // That memory is not kept for as long as the arena lives
  arena.pop_front();
  arena.pop_front();
  REQUIRE_LT(arena.allocated_bytes(), long_text.size());
  REQUIRE_EQ(after, "after");

  arena.pop_front();
  REQUIRE(arena.empty());

  // The blocks are written in a ring as before, with a block of the usual size in its place
  for (size_t i = 0; i < 50; ++i)
  {
    std::string const text = "message " + std::to_string(i);
    std::string_view const message = add_message(arena, text);
    REQUIRE_EQ(message, text);

    arena.pop_front();
    REQUIRE_LT(arena.allocated_bytes(), long_text.size());
  }

  // Also when the long message is the newest one, so that its block is the one written next
  std::string_view const only_message = add_message(arena, long_text);
  REQUIRE_EQ(only_message, long_text);

  arena.pop_front();
  REQUIRE(arena.empty());
  REQUIRE_LT(arena.allocated_bytes(), long_text.size());

  std::string_view const last_message = add_message(arena, "last");
  REQUIRE_EQ(last_message, "last");

  arena.pop_front();
  REQUIRE(arena.empty());
}

/***/
TEST_CASE("message_arena_reset_frees_the_blocks")
{
  constexpr size_t block_capacity{64};
  MessageArena arena{block_capacity};

  for (size_t i = 0; i < 10; ++i)
  {
    (void)add_message(arena, std::string(20, 'r'));
  }

  REQUIRE_FALSE(arena.empty());
  REQUIRE_EQ(arena.allocated_bytes(), 4 * block_capacity);

  arena.reset();
  REQUIRE(arena.empty());
  REQUIRE_EQ(arena.allocated_bytes(), 0);

  // It allocates again when it is used again
  std::string_view const message = add_message(arena, "after the reset");
  REQUIRE_EQ(message, "after the reset");
  REQUIRE_EQ(arena.allocated_bytes(), block_capacity);

  arena.pop_front();
  REQUIRE(arena.empty());
}

/***/
TEST_CASE("message_arena_move_keeps_the_committed_messages")
{
  constexpr size_t block_capacity{64};
  MessageArena arena{block_capacity};

  std::vector<std::string> expected;
  std::vector<std::string_view> stored;

  for (size_t i = 0; i < 7; ++i)
  {
    expected.push_back(std::string(20, static_cast<char>('a' + i)));
    stored.push_back(add_message(arena, expected.back()));
  }

  // The messages are not copied: the views into the first arena are those of the second one
  MessageArena moved_to{std::move(arena)};
  REQUIRE_FALSE(moved_to.empty());
  REQUIRE_EQ(moved_to.allocated_bytes(), 3 * block_capacity);

  expected.push_back("added after the move");
  stored.push_back(add_message(moved_to, expected.back()));
  REQUIRE_EQ(stored[7].data(), stored[6].data() + stored[6].size());

  MessageArena assigned_to{1024};
  (void)add_message(assigned_to, "replaced by the assignment");
  assigned_to = std::move(moved_to);

  expected.push_back("added after the assignment");
  stored.push_back(add_message(assigned_to, expected.back()));

  for (size_t i = 0; i < stored.size(); ++i)
  {
    REQUIRE_EQ(stored[i], expected[i]);
    REQUIRE_FALSE(assigned_to.empty());
    assigned_to.pop_front();
  }

  REQUIRE(assigned_to.empty());
}

TEST_SUITE_END();
