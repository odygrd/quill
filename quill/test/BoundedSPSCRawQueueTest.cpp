#include "doctest/doctest.h"

#include "quill/detail/spsc_queue/BoundedSPSCRawQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("BoundedSPSCRawQueue");

using namespace quill::detail;

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