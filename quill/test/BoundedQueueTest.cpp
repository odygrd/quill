#include "doctest/doctest.h"

#include "quill/detail/misc/Utilities.h"
#include "quill/detail/spsc_queue/BoundedQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("BoundedQueue");

using namespace quill::detail;

TEST_CASE("read_write_buffer")
{
  BoundedQueue<64u> buffer;

  {
    std::byte* write_buf = buffer.prepare_write(32u);
    REQUIRE_NE(write_buf, nullptr);
    buffer.commit_write(32u);
  }

  {
    auto const res = buffer.prepare_read();
    REQUIRE_EQ(std::get<1>(res), 32);
    buffer.finish_read(32u);
  }

  {
    // now we try to add another 32
    std::byte* write_buf = buffer.prepare_write(32u);
    REQUIRE_EQ(write_buf, nullptr);
  }

  {
    // Nothing to read but consumer will also wrap
    auto const res = buffer.prepare_read();
    REQUIRE_EQ(std::get<1>(res), 0);
  }

  {
    // now we try to add another 32, it should be possible after the consumer wrapped
    std::byte* write_buf = buffer.prepare_write(32u);
    REQUIRE_NE(write_buf, nullptr);
    buffer.commit_write(32u);
  }
}

TEST_CASE("read_write_multithreaded_plain_ints")
{
  BoundedQueue<QUILL_QUEUE_CAPACITY> buffer;

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
          buffer.commit_write(sizeof(uint32_t));
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
          auto [read_buffer, bytes, has_more] = buffer.prepare_read();
          while (bytes == 0)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
            std::tie(read_buffer, bytes, has_more) = buffer.prepare_read();
          }

          auto value = reinterpret_cast<uint32_t const*>(read_buffer);
          REQUIRE_EQ(*value, i);
          buffer.finish_read(sizeof(uint32_t));
        }
      }
    });

  producer_thread.join();
  consumer_thread.join();
}
TEST_SUITE_END();
