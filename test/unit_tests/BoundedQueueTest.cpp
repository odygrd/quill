#include "doctest/doctest.h"
#include "misc/DocTestExtensions.h"

#include "quill/core/BoundedSPSCQueue.h"
#include "quill/core/QuillError.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <thread>
#include <vector>

#if defined(__linux__)
  #include <sys/mman.h>
  #include <unistd.h>
#endif

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4996)
#endif

TEST_SUITE_BEGIN("BoundedQueue");

using namespace quill::detail;

TEST_CASE("read_write_buffer")
{
  BoundedSPSCQueue buffer{64u};

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
  BoundedSPSCQueueImpl<uint8_t> buffer{128, quill::HugePagesPolicy::Never, 0};
  size_t constexpr iterations = static_cast<size_t>(std::numeric_limits<uint8_t>::max()) * 8ull;

  for (size_t i = 0; i < iterations; ++i)
  {
    std::string to_write{"test"};
    to_write += std::to_string(i);
    std::byte* r = buffer.prepare_write(static_cast<uint8_t>(to_write.length()) + static_cast<uint8_t>(1));
    std::strncpy(reinterpret_cast<char*>(r), to_write.data(), to_write.length() + 1);
    buffer.finish_write(static_cast<uint8_t>(to_write.length()) + static_cast<uint8_t>(1));
    buffer.commit_write();

    // now read
    std::byte* w = buffer.prepare_read();
    REQUIRE(w);
    char result[256];
    std::memcpy(&result[0], w, to_write.length() + 1u);
    REQUIRE_STREQ(result, to_write.data());
    buffer.finish_read(static_cast<uint8_t>(to_write.length()) + static_cast<uint8_t>(1));
    buffer.commit_read();
  }
}

TEST_CASE("bounded_queue_read_write_multithreaded_plain_ints")
{
  BoundedSPSCQueue buffer{131'072};

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
          std::byte const* read_buffer = buffer.prepare_read();
          while (!read_buffer)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
            read_buffer = buffer.prepare_read();
          }

          auto const value = reinterpret_cast<uint32_t const*>(read_buffer);
          REQUIRE_EQ(*value, i);
          buffer.finish_read(sizeof(uint32_t));
          buffer.commit_read();
        }
      }
    });

  producer_thread.join();
  consumer_thread.join();
}

TEST_CASE("bounded_queue_prepare_write_larger_than_capacity_fails")
{
  BoundedSPSCQueue buffer{64u};

  REQUIRE_EQ(buffer.capacity(), 64u);
  REQUIRE_EQ(buffer.prepare_write(65u), nullptr);
  REQUIRE_EQ(buffer.prepare_write(128u), nullptr);

  std::byte* write_buf = buffer.prepare_write(32u);
  REQUIRE_NE(write_buf, nullptr);
  buffer.finish_write(32u);
  buffer.commit_write();

  std::byte* read_buf = buffer.prepare_read();
  REQUIRE_NE(read_buf, nullptr);
  buffer.finish_read(32u);
  buffer.commit_read();

  REQUIRE_EQ(buffer.prepare_write(65u), nullptr);
  REQUIRE_EQ(buffer.prepare_write(128u), nullptr);
}

TEST_CASE("bounded_queue_write_after_drain_uses_full_capacity")
{
  // default reader_store_percent 5 -> the reader publishes its position every 204 bytes
  BoundedSPSCQueue buffer{4096u};

  // consume a message smaller than the publish batch, draining the queue
  std::byte* write_buf = buffer.prepare_write(100u);
  REQUIRE_NE(write_buf, nullptr);
  buffer.finish_write(100u);
  buffer.commit_write();

  std::byte* read_buf = buffer.prepare_read();
  REQUIRE_NE(read_buf, nullptr);
  buffer.finish_read(100u);
  buffer.commit_read();

  // the queue is empty; a message that fits in the capacity must be writable. If the reader
  // position was not published on drain, the producer permanently sees only (capacity - 100)
  // free bytes and this write can never succeed
  REQUIRE_EQ(buffer.prepare_read(), nullptr);
  REQUIRE_NE(buffer.prepare_write(4000u), nullptr);
}

TEST_CASE("bounded_queue_records_start_on_8_byte_boundaries")
{
  BoundedSPSCQueue buffer{1024u};

  // Odd record sizes, written and read through more than one lap of the queue
  for (size_t i = 0; i < 300; ++i)
  {
    size_t const record_size = 1u + ((i * 7u) % 61u);

    std::byte* write_buf = buffer.prepare_write(record_size);
    REQUIRE_NE(write_buf, nullptr);
    REQUIRE_EQ(reinterpret_cast<uintptr_t>(write_buf) % 8u, 0u);
    std::memset(write_buf, static_cast<int>(i & 0xFFu), record_size);
    buffer.finish_write(record_size);
    buffer.commit_write();

    std::byte* read_buf = buffer.prepare_read();
    REQUIRE_EQ(read_buf, write_buf);
    REQUIRE_EQ(read_buf[record_size - 1u], static_cast<std::byte>(i & 0xFFu));
    buffer.finish_read(record_size);
    buffer.commit_read();

    REQUIRE_EQ(buffer.bytes_read() % 8u, 0u);
  }

  REQUIRE_EQ(buffer.prepare_read(), nullptr);
}

TEST_CASE("bounded_queue_rejects_a_record_that_rounding_up_would_wrap")
{
  BoundedSPSCQueue buffer{64u};

  // Rounded up to the record alignment, SIZE_MAX would wrap to 0 and look as if it fits
  REQUIRE_EQ(buffer.prepare_write(SIZE_MAX), nullptr);
  REQUIRE_EQ(buffer.prepare_write_reserve_cached(SIZE_MAX).write_buffer, nullptr);
  REQUIRE_EQ(buffer.prepare_write(65u), nullptr);
  REQUIRE_NE(buffer.prepare_write(64u), nullptr);
}

#if !defined(QUILL_NO_EXCEPTIONS)
TEST_CASE("oversized_capacity_throws_before_allocating")
{
  size_t constexpr oversized_capacity = (std::numeric_limits<size_t>::max() / 2u) + 1u;
  REQUIRE_THROWS_AS(BoundedSPSCQueue{oversized_capacity}, quill::QuillError);
}
#endif

/***/
TEST_CASE("bounded_queue_prefaults_the_start_of_its_scratch_space")
{
#if !defined(__linux__)
  return;
#else
  // A record that crosses the ring boundary is written to the scratch space after the logical
  // queue region. The first one must not have to fault the page of that space in
  constexpr size_t capacity{64u * 1024u};
  BoundedSPSCQueue buffer{capacity};

  std::byte* const storage = buffer.prepare_write(8u);
  REQUIRE(storage);

  long const page_size = ::sysconf(_SC_PAGESIZE);
  REQUIRE_GT(page_size, 0);

  // The last page of the logical queue region
  unsigned char resident{0};
  REQUIRE_EQ(
    ::mincore(storage + capacity - static_cast<size_t>(page_size), static_cast<size_t>(page_size), &resident), 0);
  REQUIRE_EQ(resident & 1u, 1u);

  // The first page of the scratch space
  resident = 0;
  REQUIRE_EQ(::mincore(storage + capacity, static_cast<size_t>(page_size), &resident), 0);
  REQUIRE_EQ(resident & 1u, 1u);
#endif
}

TEST_SUITE_END();

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif
