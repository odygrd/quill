#include "doctest/doctest.h"

#include "quill/detail/spsc_queue/BoundedSPSCRawQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("BoundedSPSCRawQueue");

using namespace quill::detail;

TEST_CASE("read_write_raw_queue_single_thread")
{
  BoundedSPSCRawQueue buffer;

  // eg. (8192 times * 4) + 1
  // There is always 1 byte that is never reserved in our queue
  size_t const iterations_to_full = (buffer.capacity() / sizeof(uint32_t)) - 2;

  for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
  {
    // buffer is empty
    REQUIRE_EQ(buffer.empty(), true);

    // we do one iteration because we need to make producer/consumer update their positions
    // when the buffer wraps around
    auto* write_buffer = buffer.prepare_write(sizeof(uint32_t));
    REQUIRE_NE(write_buffer, nullptr);
    std::memcpy(write_buffer, &wrap_cnt, sizeof(uint32_t));
    buffer.commit_write(sizeof(uint32_t));

    auto read_buffer = buffer.prepare_read();
    REQUIRE_NE(read_buffer.second, 0);
    auto value = reinterpret_cast<uint32_t const*>(read_buffer.first);
    REQUIRE_EQ(*value, wrap_cnt);
    buffer.finish_read(sizeof(uint32_t));

    // Main test loop
    for (uint32_t i = 0; i < iterations_to_full; ++i)
    {
      auto* write_buffer = buffer.prepare_write(sizeof(uint32_t));

      while (!write_buffer)
      {
        std::this_thread::sleep_for(std::chrono::microseconds{2});
        write_buffer = buffer.prepare_write(sizeof(uint32_t));
      }

      std::memcpy(write_buffer, &i, sizeof(uint32_t));

      buffer.commit_write(sizeof(uint32_t));
    }

    for (uint32_t i = 0; i < iterations_to_full; ++i)
    {
      auto read_buffer = buffer.prepare_read();
      while (read_buffer.second == 0)
      {
        std::this_thread::sleep_for(std::chrono::microseconds{2});
        read_buffer = buffer.prepare_read();
      }

      auto value = reinterpret_cast<uint32_t const*>(read_buffer.first);
      REQUIRE_EQ(*value, i);

      buffer.finish_read(sizeof(uint32_t));
    }
  }
}

TEST_CASE("read_write_multithreaded_raw_queue")
{
  BoundedSPSCRawQueue buffer;

  std::thread producer_thread([&buffer]() {
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

        buffer.commit_write(sizeof(uint32_t));
      }
    }
  });

  std::thread consumer_thread([&buffer]() {
    for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 8192; ++i)
      {
        auto read_buffer = buffer.prepare_read();
        while (read_buffer.second == 0)
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          read_buffer = buffer.prepare_read();
        }

        auto value = reinterpret_cast<uint32_t const*>(read_buffer.first);
        REQUIRE_EQ(*value, i);

        buffer.finish_read(sizeof(uint32_t));
      }
    }
  });

  producer_thread.join();
  consumer_thread.join();
}
TEST_SUITE_END();