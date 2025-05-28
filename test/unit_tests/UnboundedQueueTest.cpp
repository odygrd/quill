#include "doctest/doctest.h"
#include "quill/core/FrontendOptions.h"

#include "quill/core/UnboundedSPSCQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("UnboundedQueue");

using namespace quill::detail;

TEST_CASE("unbounded_queue_shrink")
{
  constexpr size_t CHUNK{256};
  constexpr size_t INITIAL_SIZE{1024};

  UnboundedSPSCQueue buffer{INITIAL_SIZE, quill::FrontendOptions::unbounded_queue_max_capacity};
  REQUIRE_EQ(buffer.producer_capacity(), INITIAL_SIZE);

  // This queue will grow as we request 5 * 256
  for (uint32_t i = 0; i < 5; ++i)
  {
    auto* write_buffer = buffer.prepare_write(CHUNK);

    REQUIRE(write_buffer);
    buffer.finish_write(CHUNK);
    buffer.commit_write();
  }

  {
    UnboundedSPSCQueue::ReadResult res = buffer.prepare_read();
    REQUIRE(res.read_pos);
    buffer.finish_read(INITIAL_SIZE);
    buffer.commit_read();
  }

  {
    // Now read again the next part from the allocated queue
    UnboundedSPSCQueue::ReadResult res = buffer.prepare_read();
    REQUIRE(res.read_pos);
    REQUIRE(res.allocation);
    REQUIRE_EQ(res.previous_capacity, INITIAL_SIZE);
    REQUIRE_EQ(res.new_capacity, INITIAL_SIZE * 2);
    buffer.finish_read(CHUNK);
    buffer.commit_read();
  }

  // Shrink the queue
  REQUIRE_EQ(buffer.producer_capacity(), INITIAL_SIZE * 2);
  buffer.shrink(INITIAL_SIZE);
  REQUIRE_EQ(buffer.producer_capacity(), INITIAL_SIZE);

  {
    // On next read we should see the new queue - old is gone
    UnboundedSPSCQueue::ReadResult res = buffer.prepare_read();
    REQUIRE_EQ(res.read_pos, nullptr);
    REQUIRE(res.allocation);
    REQUIRE_EQ(res.previous_capacity, INITIAL_SIZE * 2);
    REQUIRE_EQ(res.new_capacity, INITIAL_SIZE);
    buffer.finish_read(CHUNK);
    buffer.commit_read();
  }
}

TEST_CASE("unbounded_queue_allocation_within_limit")
{
  UnboundedSPSCQueue buffer{1024, std::numeric_limits<uint32_t>::max()};

  static constexpr size_t two_mb = 2u * 1024u * 1024u;

  // Attempt to allocate a buffer size that exceeds the default limit,
  // ensuring that allocation within configurable bounds does not throw.
  auto func = [&buffer]()
  {
    auto* write_buffer_z = buffer.prepare_write(2 * two_mb);
    return write_buffer_z;
  };

  REQUIRE_NOTHROW(func());
  REQUIRE_EQ(buffer.producer_capacity(), 2 * two_mb);
}

TEST_CASE("unbounded_queue_allocation_exceeds_limit")
{
  constexpr static uint64_t two_mb = 2u * 1024u * 1024u;

  UnboundedSPSCQueue buffer{1024, two_mb};

  // Attempt to allocate a buffer size that exceeds the specified capacity,
  // which should trigger an exception.
  auto func = [&buffer]()
  {
    auto* write_buffer_z = buffer.prepare_write(2 * two_mb);
    return write_buffer_z;
  };

  REQUIRE_THROWS_AS(func(), quill::QuillError);
  REQUIRE_EQ(buffer.producer_capacity(), 1024);
}

TEST_CASE("unbounded_queue_read_write_multithreaded_plain_ints")
{
  UnboundedSPSCQueue buffer{1024, quill::FrontendOptions::unbounded_queue_max_capacity};

  std::thread producer_thread(
    [&buffer]()
    {
      for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
      {
        for (uint32_t i = 0; i < 8192; ++i)
        {
          auto* write_buffer = buffer.prepare_write(sizeof(uint32_t));

          while (!write_buffer)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});

            write_buffer = buffer.prepare_write(sizeof(uint32_t));
          }

          std::memcpy(write_buffer, &i, sizeof(uint32_t));
          buffer.finish_write(sizeof(uint32_t));
          buffer.commit_write();
        }
      }
    });

  // Delay creating the consumer thread
  std::this_thread::sleep_for(std::chrono::milliseconds{300});

  std::thread consumer_thread(
    [&buffer]()
    {
      for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
      {
        for (uint32_t i = 0; i < 8192; ++i)
        {
          auto read_result = buffer.prepare_read();
          while (!read_result.read_pos)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
            read_result = buffer.prepare_read();
          }

          auto const value = reinterpret_cast<uint32_t const*>(read_result.read_pos);
          REQUIRE_EQ(*value, i);
          buffer.finish_read(sizeof(uint32_t));
          buffer.commit_read();
        }
      }
    });

  producer_thread.join();
  consumer_thread.join();
  REQUIRE(buffer.empty());
}

TEST_SUITE_END();
