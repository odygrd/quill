#include "doctest/doctest.h"
#include "misc/DocTestExtensions.h"

#include "quill/detail/misc/Utilities.h"
#include "quill/detail/spsc_queue/BoundedQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("BoundedQueue");

using namespace quill::detail;

#if !defined(QUILL_X86ARCH)
// QUILL_X86ARCH requires at least a queue capacity of 1024 and those tests are using a smaller number

TEST_CASE("read_write_buffer")
{
  BoundedQueue buffer{64u};

  for (uint32_t i = 0; i < 128; ++i)
  {
    {
      std::byte* write_buf = buffer.prepare_write(32u);
      REQUIRE_NE(write_buf, nullptr);
      buffer.finish_write(32u);
      buffer.commit_write();
    }

    {
      std::byte* write_buf = buffer.prepare_write(32u);
      REQUIRE_NE(write_buf, nullptr);
      buffer.finish_write(32u);
      buffer.commit_write();
    }

    {
      std::byte* res = buffer.prepare_read();
      REQUIRE(res);
      buffer.finish_read(32u);
      buffer.commit_read();
    }

    {
      std::byte* res = buffer.prepare_read();
      REQUIRE(res);
      buffer.finish_read(32u);
      buffer.commit_read();

      res = buffer.prepare_read();
      REQUIRE_FALSE(res);
    }
  }

  std::byte* res = buffer.prepare_read();
  REQUIRE_FALSE(res);
}

TEST_CASE("bounded_queue_integer_overflow")
{
  BoundedQueueImpl<uint8_t> buffer{128};
  size_t const iterations = static_cast<size_t>(std::numeric_limits<uint8_t>::max()) * 8ull;

  for (size_t i = 0; i < iterations; ++i)
  {
    std::string to_write{"test"};
    to_write += std::to_string(i);
    std::byte* r = buffer.prepare_write(static_cast<uint8_t>(to_write.length()) + 1);
    std::strncpy(reinterpret_cast<char*>(r), to_write.data(), to_write.length() + 1);
    buffer.finish_write(static_cast<uint8_t>(to_write.length()) + 1);
    buffer.commit_write();

    // now read
    std::byte* w = buffer.prepare_read();
    REQUIRE(w);
    char result[256];
    std::memcpy(&result[0], w, static_cast<uint8_t>(to_write.length()) + 1);
    REQUIRE_STREQ(result, to_write.data());
    buffer.finish_read(static_cast<uint8_t>(to_write.length()) + 1);
    buffer.commit_read();
  }
}
#endif

TEST_CASE("bounded_queue_read_write_multithreaded_plain_ints")
{
  BoundedQueue buffer{131'072};

  std::thread producer_thread(
    [&buffer]()
    {
      for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
      {
        for (uint32_t i = 0; i < 8192; ++i)
        {
          std::byte* write_buffer = buffer.prepare_write(sizeof(uint32_t));

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

  std::thread consumer_thread(
    [&buffer]()
    {
      for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
      {
        for (uint32_t i = 0; i < 8192; ++i)
        {
          std::byte* read_buffer = buffer.prepare_read();
          while (!read_buffer)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
            read_buffer = buffer.prepare_read();
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
}

TEST_SUITE_END();
