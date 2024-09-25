#include "doctest/doctest.h"

#include "quill/core/UnboundedSPSCQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("UnboundedQueue");

using namespace quill::detail;

TEST_CASE("unbounded_queue_max_limit")
{
  UnboundedSPSCQueue buffer{1024};

  constexpr static uint64_t half_gb = 500u * 1024u * 1024u;
  constexpr static uint64_t two_gb = 2u * 1024u * 1024u * 1024u - 1;

#if defined(_MSC_VER)
  auto* write_buffer_a = buffer.prepare_write(half_gb, quill::QueueType::UnboundedUnlimited);
#else
  auto* write_buffer_a = buffer.prepare_write<quill::QueueType::UnboundedUnlimited>(half_gb);
#endif
  REQUIRE(write_buffer_a);
  buffer.finish_write(half_gb);
  buffer.commit_write();

#if defined(_MSC_VER)
  auto* write_buffer_b = buffer.prepare_write(two_gb, quill::QueueType::UnboundedUnlimited);
#else
  auto* write_buffer_b = buffer.prepare_write<quill::QueueType::UnboundedUnlimited>(two_gb);
#endif
  REQUIRE(write_buffer_b);
  buffer.finish_write(two_gb);
  buffer.commit_write();

  // Buffer is filled with two GB here, we can try to reserve more to allocate another queue
#if defined(_MSC_VER)
  auto* write_buffer_c = buffer.prepare_write(two_gb, quill::QueueType::UnboundedBlocking);
#else
  auto* write_buffer_c = buffer.prepare_write<quill::QueueType::UnboundedBlocking>(two_gb);
#endif
  REQUIRE_FALSE(write_buffer_c);

#if defined(_MSC_VER)
  write_buffer_c = buffer.prepare_write(two_gb, quill::QueueType::UnboundedDropping);
#else
  write_buffer_c = buffer.prepare_write<quill::QueueType::UnboundedDropping>(two_gb);
#endif
  REQUIRE_FALSE(write_buffer_c);

  // Buffer is filled with two GB here, we can try to reserve more to allocate another queue
  // for the UnboundedLimit queue
#if defined(_MSC_VER)
  write_buffer_c = buffer.prepare_write(two_gb, quill::QueueType::UnboundedUnlimited);
#else
  write_buffer_c = buffer.prepare_write<quill::QueueType::UnboundedUnlimited>(two_gb);
#endif
  REQUIRE(write_buffer_c);
  buffer.finish_write(two_gb);
  buffer.commit_write();

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

// These tests work but sometimes can randomly crash on CI
// TEST_CASE("unbounded_queue_unbounded_unlimited")
//{
//  UnboundedSPSCQueue buffer{1024};
//
//  // Try to allocate over 2GB
//  auto func = [&buffer]()
//  {
//    auto* write_buffer_z = buffer.prepare_write<quill::QueueType::UnboundedUnlimited>(three_gb);
//    return write_buffer_z;
//  };
//  REQUIRE_NOTHROW(func());
//}
//
// TEST_CASE("unbounded_queue_unbounded_blocking")
//{
//  UnboundedSPSCQueue buffer{1024};
//
//  // Try to allocate over 2GB
//  auto func = [&buffer]()
//  {
//    auto* write_buffer_z = buffer.prepare_write<quill::QueueType::UnboundedBlocking>(three_gb);
//    return write_buffer_z;
//  };
//  REQUIRE_THROWS_AS(func(), quill::QuillError);
//}
//
// TEST_CASE("unbounded_queue_unbounded_dropping")
//{
//  UnboundedSPSCQueue buffer{1024};
//
//  // Try to allocate over 2GB
//  auto func = [&buffer]()
//  {
//    auto* write_buffer_z = buffer.prepare_write<quill::QueueType::UnboundedDropping>(three_gb);
//    return write_buffer_z;
//  };
//  REQUIRE_THROWS_AS(func(), quill::QuillError);
//}

TEST_SUITE_END();