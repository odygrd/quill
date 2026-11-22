#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <cstddef>
#include <new>
#include <string>
#include <vector>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
// While this is not zero, the calling thread cannot allocate that many bytes or more
thread_local std::size_t fail_allocations_from_size{0};

// The allocation that follows a failed one fails too, whatever its size, when this is set: the
// memory for the message that reports the failure
thread_local bool fail_allocation_after_failure{false};
thread_local bool fail_next_allocation{false};
} // namespace

// This executable replaces global operator new to simulate memory that is not available for a
// large record, without exhausting memory. Other test executables use their normal allocator.
// The array forms are replaced too: a sanitizer runtime does not pass them on to the scalar ones.
namespace
{
void* allocate(std::size_t size)
{
  if (fail_next_allocation)
  {
    fail_next_allocation = false;
    throw std::bad_alloc{};
  }

  if ((fail_allocations_from_size != 0) && (size >= fail_allocations_from_size))
  {
    fail_next_allocation = fail_allocation_after_failure;
    throw std::bad_alloc{};
  }

  return ::operator new(size, std::align_val_t{alignof(std::max_align_t)});
}

void deallocate(void* memory) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
} // namespace

void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }

void operator delete(void* memory) noexcept { deallocate(memory); }
void operator delete(void* memory, std::size_t) noexcept { deallocate(memory); }
void operator delete[](void* memory) noexcept { deallocate(memory); }
void operator delete[](void* memory, std::size_t) noexcept { deallocate(memory); }
#endif

/***/
TEST_CASE("log_binary_allocation_failure_drops_the_record_and_writes_the_others")
{
#if defined(QUILL_NO_EXCEPTIONS) || QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#elif defined(_ITERATOR_DEBUG_LEVEL) && (_ITERATOR_DEBUG_LEVEL > 0)
  // The MSVC debug STL allocates container proxies inside noexcept moves, where an injected
  // bad_alloc terminates the process before it can reach the backend's error handling
  return;
#else
  class CapturingSink : public quill::Sink
  {
  public:
    void write_log(quill::LogRecord const& record) override
    {
      events.push_back("log:" + std::string{record.log_message});
    }

    void write_binary(quill::BinaryRecord const& record) override
    {
      events.push_back("binary:" + std::to_string(record.size));
    }

    void flush_sink(quill::SinkFlushReason) override {}

    std::vector<std::string> events;
  };

  size_t errors{0};
  quill::BackendOptions options;
  options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  options.error_notifier = [&errors](std::string const&) { ++errors; };
  auto* worker = quill::Backend::acquire_manual_backend_worker();
  worker->init(options);

  auto sink =
    quill::Frontend::create_or_get_sink<CapturingSink>("log_binary_allocation_failure_sink");
  auto* logger = quill::Frontend::create_or_get_logger("log_binary_allocation_failure_logger", sink);

  // Longer than a block of the backend's messages, so the backend allocates memory for it
  std::string const large_record(48u * 1024u, 'b');
  std::string const small_record(16, 's');

  // Discover this thread's context and grow the backend's scratch buffers
  LOG_INFO(logger, "warmup");
  LOG_BINARY(logger, small_record.data(), small_record.size());
  worker->poll();
  REQUIRE_EQ(sink->events.size(), 2u);
  sink->events.clear();

  LOG_INFO(logger, "before");
  LOG_BINARY(logger, large_record.data(), large_record.size());
  LOG_INFO(logger, "after");
  LOG_BINARY(logger, small_record.data(), small_record.size());

  // The backend cannot get the memory of the large record, however often it polls. The record is
  // dropped and reported, and the events around it are written
  fail_allocations_from_size = 40u * 1024u;
  worker->poll();
  worker->poll();
  worker->poll();
  fail_allocations_from_size = 0;

  REQUIRE_EQ(errors, 1u);
  REQUIRE_EQ(sink->events.size(), 3u);
  CHECK_EQ(sink->events[0], "log:before");
  CHECK_EQ(sink->events[1], "log:after");
  CHECK_EQ(sink->events[2], "binary:16");
  sink->events.clear();

  // With the memory available again, a large record is written
  LOG_BINARY(logger, large_record.data(), large_record.size());
  worker->poll();

  REQUIRE_EQ(errors, 1u);
  REQUIRE_EQ(sink->events.size(), 1u);
  CHECK_EQ(sink->events[0], "binary:" + std::to_string(large_record.size()));
  sink->events.clear();

  // There is no memory for the message that reports the dropped record either. Reporting is
  // best effort: the record is still dropped, and the events around it are written
  LOG_INFO(logger, "before");
  LOG_BINARY(logger, large_record.data(), large_record.size());
  LOG_INFO(logger, "after");

  fail_allocations_from_size = 40u * 1024u;
  fail_allocation_after_failure = true;
  worker->poll();
  worker->poll();
  worker->poll();
  fail_allocations_from_size = 0;
  fail_allocation_after_failure = false;

  REQUIRE_FALSE(fail_next_allocation);
  REQUIRE_EQ(errors, 1u);
  REQUIRE_EQ(sink->events.size(), 2u);
  CHECK_EQ(sink->events[0], "log:before");
  CHECK_EQ(sink->events[1], "log:after");

  worker->shutdown();
  quill::Frontend::remove_logger(logger);
#endif
}
