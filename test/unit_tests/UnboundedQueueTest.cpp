#include "doctest/doctest.h"

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

  UnboundedSPSCQueue buffer{INITIAL_SIZE};

  // This queue will grow as we request 5 * 256
  for (uint32_t i = 0; i < 5; ++i)
  {
    #if defined(_MSC_VER)
    auto* write_buffer = buffer.prepare_write(CHUNK, quill::QueueType::UnboundedBlocking);
    #else
    auto* write_buffer = buffer.prepare_write<quill::QueueType::UnboundedBlocking>(CHUNK);
    #endif

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
  buffer.shrink(INITIAL_SIZE);

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

TEST_CASE("unbounded_queue_read_write_multithreaded_plain_ints")
{
  UnboundedSPSCQueue buffer{1024};

  std::thread producer_thread(
    [&buffer]()
    {
      for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
      {
        for (uint32_t i = 0; i < 8192; ++i)
        {
#if defined(_MSC_VER)
          auto* write_buffer = buffer.prepare_write(sizeof(uint32_t), quill::QueueType::UnboundedBlocking);
#else
          auto* write_buffer = buffer.prepare_write<quill::QueueType::UnboundedBlocking>(sizeof(uint32_t));
#endif

          while (!write_buffer)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
#if defined(_MSC_VER)
            write_buffer = buffer.prepare_write(sizeof(uint32_t), quill::QueueType::UnboundedBlocking);
#else
            write_buffer = buffer.prepare_write<quill::QueueType::UnboundedBlocking>(sizeof(uint32_t));
#endif
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