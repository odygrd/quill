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
          auto* write_buffer = buffer.prepare_write(sizeof(uint32_t), quill::QueueType::UnboundedBlocking);

          while (!write_buffer)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
            write_buffer = buffer.prepare_write(sizeof(uint32_t), quill::QueueType::UnboundedBlocking);
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

TEST_CASE("unbounded_queue_max_limit")
{
  UnboundedSPSCQueue buffer{1024};

  constexpr uint64_t half_gb = 500u * 1024u * 1024u;
  constexpr uint64_t two_gb = 2u * 1024u * 1024u * 1024u - 1;
  constexpr uint64_t three_gb = 3u * 1024u * 1024u * 1024u;

  auto* write_buffer_a = buffer.prepare_write(half_gb, quill::QueueType::UnboundedUnlimited);
  REQUIRE(write_buffer_a);
  buffer.finish_write(half_gb);
  buffer.commit_write();

  auto* write_buffer_b = buffer.prepare_write(two_gb, quill::QueueType::UnboundedUnlimited);
  REQUIRE(write_buffer_b);
  buffer.finish_write(two_gb);
  buffer.commit_write();

  // Buffer is filled with two GB here, we can try to reserve more to allocate another queue
  auto* write_buffer_c = buffer.prepare_write(two_gb, quill::QueueType::UnboundedBlocking);
  REQUIRE_FALSE(write_buffer_c);

  write_buffer_c = buffer.prepare_write(two_gb, quill::QueueType::UnboundedDropping);
  REQUIRE_FALSE(write_buffer_c);

  // Buffer is filled with two GB here, we can try to reserve more to allocate another queue
  // for the UnboundedLimit queue
  write_buffer_c = buffer.prepare_write(two_gb, quill::QueueType::UnboundedUnlimited);
  REQUIRE(write_buffer_c);
  buffer.finish_write(two_gb);
  buffer.commit_write();

  // Try to allocate over 2GB
  REQUIRE_THROWS_AS(QUILL_MAYBE_UNUSED auto* write_buffer_z =
                      buffer.prepare_write(three_gb, quill::QueueType::UnboundedUnlimited),
                    quill::QuillError);
  
  auto read_result_a = buffer.prepare_read();
  REQUIRE(read_result_a.read_pos);
  buffer.finish_read(half_gb);
  buffer.commit_read();

  auto read_result_b = buffer.prepare_read();
  REQUIRE(read_result_b.read_pos);
  buffer.finish_read(two_gb);
  buffer.commit_read();

  auto read_result_c = buffer.prepare_read();
  REQUIRE(read_result_c.read_pos);
  buffer.finish_read(two_gb);
  buffer.commit_read();

  REQUIRE(buffer.empty());
}
TEST_SUITE_END();