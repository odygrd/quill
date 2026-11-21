#include "DocTestExtensions.h"
#include "doctest/doctest.h"

#include "quill/backend/BacktraceStorage.h"

#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>
#include <vector>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
thread_local bool fail_next_allocation{false};
} // namespace

// This executable replaces global operator new to simulate a single allocation failure
// without exhausting memory. Other test executables use their normal allocator.
void* operator new(std::size_t size)
{
  if (fail_next_allocation)
  {
    fail_next_allocation = false;
    throw std::bad_alloc{};
  }

  return ::operator new(size, std::align_val_t{alignof(std::max_align_t)});
}

void operator delete(void* memory) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
void operator delete(void* memory, std::size_t) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
#endif

TEST_SUITE_BEGIN("BacktraceStorage");

using namespace quill::detail;

TEST_CASE("store_noops_when_capacity_is_zero")
{
  BacktraceStorage backtrace_storage;

  backtrace_storage.store(TransitEvent{}, "thread_1", "worker");

  size_t callback_invocations = 0;
  backtrace_storage.process([&callback_invocations](TransitEvent const&, std::string_view, std::string_view)
                            { ++callback_invocations; });

  REQUIRE_EQ(callback_invocations, 0);
}

TEST_CASE("set_capacity_zero_clears_existing_storage")
{
  BacktraceStorage backtrace_storage;
  backtrace_storage.set_capacity(2);

  backtrace_storage.store(TransitEvent{}, "thread_1", "worker");
  backtrace_storage.store(TransitEvent{}, "thread_2", "worker");

  backtrace_storage.set_capacity(0);
  backtrace_storage.store(TransitEvent{}, "thread_3", "worker");

  size_t callback_invocations = 0;
  backtrace_storage.process([&callback_invocations](TransitEvent const&, std::string_view, std::string_view)
                            { ++callback_invocations; });

  REQUIRE_EQ(callback_invocations, 0);
}

TEST_CASE("store_keeps_a_copy_of_the_message")
{
  BacktraceStorage backtrace_storage;
  backtrace_storage.set_capacity(4);

  // The backend reuses the transit event and the memory of its message after storing it
  std::string message{"first backtrace message"};
  TransitEvent transit_event;
  transit_event.timestamp = 1;
  transit_event.formatted_msg = message;
  backtrace_storage.store(transit_event, "thread_1", "worker_1");

  message = "second backtrace message";
  transit_event.timestamp = 2;
  transit_event.formatted_msg = message;
  backtrace_storage.store(transit_event, "thread_2", "worker_2");

  message = "overwritten after it was stored";
  transit_event.formatted_msg = std::string_view{};

  std::vector<std::string> stored_messages;
  std::vector<std::string> stored_thread_ids;
  backtrace_storage.process(
    [&stored_messages, &stored_thread_ids](TransitEvent const& stored_event,
                                           std::string_view thread_id, std::string_view thread_name)
    {
      REQUIRE_EQ(stored_event.timestamp, stored_messages.size() + 1);
      REQUIRE_EQ(thread_name, "worker_" + std::to_string(stored_event.timestamp));
      stored_messages.emplace_back(stored_event.formatted_msg);
      stored_thread_ids.emplace_back(thread_id);
    });

  REQUIRE_EQ(stored_messages.size(), 2);
  REQUIRE_EQ(stored_messages[0], "first backtrace message");
  REQUIRE_EQ(stored_messages[1], "second backtrace message");
  REQUIRE_EQ(stored_thread_ids[0], "thread_1");
  REQUIRE_EQ(stored_thread_ids[1], "thread_2");

  // The events are stored until they are processed once
  size_t callback_invocations = 0;
  backtrace_storage.process([&callback_invocations](TransitEvent const&, std::string_view, std::string_view)
                            { ++callback_invocations; });

  REQUIRE_EQ(callback_invocations, 0);
}

TEST_CASE("set_capacity_is_set_again_after_an_allocation_failure")
{
#if defined(QUILL_NO_EXCEPTIONS) || QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#elif defined(_ITERATOR_DEBUG_LEVEL) && (_ITERATOR_DEBUG_LEVEL > 0)
  // The MSVC debug STL also allocates container proxies, so another allocation would fail
  return;
#else
  BacktraceStorage backtrace_storage;

  // Reserving the events fails
  bool failed{false};
  fail_next_allocation = true;

  try
  {
    backtrace_storage.set_capacity(3);
  }
  catch (std::bad_alloc const&)
  {
    failed = true;
  }

  fail_next_allocation = false;
  REQUIRE(failed);

  // The capacity was not set, so setting it again reserves them
  backtrace_storage.set_capacity(3);

  std::string const first_message{"first"};
  TransitEvent transit_event;
  transit_event.timestamp = 1;
  transit_event.formatted_msg = first_message;
  backtrace_storage.store(transit_event, "thread_1", "worker");

  // store() adds an event after its message, so adding the event must not need an allocation
  // that can fail. The storage has the memory of this message since the first one
  std::string const second_message{"second"};
  transit_event.timestamp = 2;
  transit_event.formatted_msg = second_message;
  fail_next_allocation = true;
  backtrace_storage.store(transit_event, "thread_2", "worker");

  bool const allocated = !fail_next_allocation;
  fail_next_allocation = false;
  REQUIRE_FALSE(allocated);

  std::vector<std::string> stored_messages;
  backtrace_storage.process([&stored_messages](TransitEvent const& stored_event, std::string_view, std::string_view)
                            { stored_messages.emplace_back(stored_event.formatted_msg); });

  REQUIRE_EQ(stored_messages.size(), 2);
  REQUIRE_EQ(stored_messages[0], "first");
  REQUIRE_EQ(stored_messages[1], "second");
#endif
}

TEST_CASE("store_replaces_the_oldest_event_and_its_message")
{
  constexpr size_t capacity{5};
  constexpr size_t stored_events{200};

  BacktraceStorage backtrace_storage;
  backtrace_storage.set_capacity(static_cast<uint32_t>(capacity));

  // Messages of 0 to 5970 bytes, so that some of them are longer than a block of the storage and
  // the events that are kept have their messages in several blocks
  auto const make_message = [](size_t index)
  { return std::string(index * 30, static_cast<char>('a' + (index % 26))); };

  for (size_t i = 0; i < stored_events; ++i)
  {
    std::string const message = make_message(i);

    TransitEvent transit_event;
    transit_event.timestamp = i;
    transit_event.formatted_msg = message;
    backtrace_storage.store(transit_event, std::to_string(i), "worker");
  }

  // The last events are kept, from the oldest to the newest
  size_t expected_index = stored_events - capacity;
  backtrace_storage.process(
    [&expected_index, &make_message](TransitEvent const& stored_event, std::string_view thread_id, std::string_view)
    {
      REQUIRE_EQ(stored_event.timestamp, expected_index);
      REQUIRE_EQ(thread_id, std::to_string(expected_index));
      REQUIRE_EQ(stored_event.formatted_msg, make_message(expected_index));
      ++expected_index;
    });

  REQUIRE_EQ(expected_index, stored_events);

  // It stores events again after they were processed
  std::string const message{"stored after the flush"};
  TransitEvent transit_event;
  transit_event.formatted_msg = message;
  backtrace_storage.store(transit_event, "thread_1", "worker");

  size_t callback_invocations = 0;
  backtrace_storage.process(
    [&callback_invocations](TransitEvent const& stored_event, std::string_view, std::string_view)
    {
      REQUIRE_EQ(stored_event.formatted_msg, "stored after the flush");
      ++callback_invocations;
    });

  REQUIRE_EQ(callback_invocations, 1);
}

TEST_SUITE_END();
