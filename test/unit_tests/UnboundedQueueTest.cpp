#include "doctest/doctest.h"

#include "quill/core/UnboundedSPSCQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("UnboundedQueue");

using namespace quill::detail;

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

          REQUIRE(write_buffer);

          std::memcpy(write_buffer, &i, sizeof(uint32_t));
          buffer.finish_write(sizeof(uint32_t));
          buffer.commit_write();
        }
      }
    });

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

TEST_CASE("unbounded_queue_read_write_multithreaded_plain_ints_produce_fully")
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

          REQUIRE(write_buffer);

          std::memcpy(write_buffer, &i, sizeof(uint32_t));
          buffer.finish_write(sizeof(uint32_t));
          buffer.commit_write();
        }
      }
    });

  // Produce everything before starting the consumer
  producer_thread.join();

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

  consumer_thread.join();
  REQUIRE(buffer.empty());
}

TEST_CASE("unbounded_queue_read_write_multithreaded_plain_ints_consume_quickly")
{
  UnboundedSPSCQueue buffer{1024};

  // Start the consumer first
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
            std::this_thread::sleep_for(std::chrono::nanoseconds{100});
            read_result = buffer.prepare_read();
          }

          auto const value = reinterpret_cast<uint32_t const*>(read_result.read_pos);
          REQUIRE_EQ(*value, i);
          buffer.finish_read(sizeof(uint32_t));
          buffer.commit_read();
        }
      }
    });

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

          REQUIRE(write_buffer);

          std::memcpy(write_buffer, &i, sizeof(uint32_t));
          buffer.finish_write(sizeof(uint32_t));
          buffer.commit_write();
        }
      }
    });

  producer_thread.join();
  consumer_thread.join();
  REQUIRE(buffer.empty());
}

TEST_SUITE_END();