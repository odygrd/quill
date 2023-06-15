#include "doctest/doctest.h"

#include "quill/detail/misc/Utilities.h"
#include "quill/detail/spsc_queue/UnboundedQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("UnboundedQueue");

using namespace quill::detail;

TEST_CASE("unbounded_queue_read_write_multithreaded_plain_ints")
{
  UnboundedQueue buffer{1024};

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
          auto [read_buffer, alloc] = buffer.prepare_read();
          while (!read_buffer)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
            std::tie(read_buffer, alloc) = buffer.prepare_read();
          }

          auto value = reinterpret_cast<uint32_t const*>(read_buffer);
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