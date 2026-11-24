#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <cstddef>
#include <new>
#include <thread>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
// Only the calling thread's next allocation fails; producer allocations are unaffected.
thread_local bool fail_next_allocation{false};
} // namespace

// This executable replaces global operator new to simulate a single allocation failure
// without exhausting memory. Other test executables use their normal allocator.
void* operator new(std::size_t size)
{
  if (fail_next_allocation)
  {
    // Consume the flag before throwing so error reporting and the next poll can allocate.
    fail_next_allocation = false;
    throw std::bad_alloc{};
  }

  return ::operator new(size, std::align_val_t{alignof(std::max_align_t)});
}

void operator delete(void* memory) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
void operator delete(void* memory, std::size_t) noexcept
{
  ::operator delete(memory, std::align_val_t{alignof(std::max_align_t)});
}
#endif

TEST_CASE("backend_retries_context_discovery_after_allocation_failure")
{
#if defined(QUILL_NO_EXCEPTIONS) || QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#else
  class CountingSink : public quill::Sink
  {
  public:
    void write_log(quill::LogRecord const& /* record */) override { ++writes; }
    void flush_sink(quill::SinkFlushReason) override {}
    size_t writes{0};
  };

  size_t errors{0};
  quill::BackendOptions options;
  options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  options.wait_for_queues_to_empty_before_exit = false;
  options.error_notifier = [&errors](std::string const&) { ++errors; };
  auto* worker = quill::Backend::acquire_manual_backend_worker();
  worker->init(options);

  auto sink = quill::Frontend::create_or_get_sink<CountingSink>("context_allocation_failure_sink");
  auto* logger = quill::Frontend::create_or_get_logger("context_allocation_failure_logger", sink);
  std::thread producer([logger]() { LOG_INFO(logger, "pending before allocation failure"); });
  producer.join();

  // The producer has registered its queue, but the manual backend has not discovered it yet.
  // poll_one() runs on this test thread. Its next allocation creates the new context's transit
  // buffer, so arming the operator new override above makes that discovery fail once.
  fail_next_allocation = true;
  worker->poll_one();
  CHECK_EQ(errors, 1u);
  CHECK_EQ(sink->writes, 0u);

  // A failed discovery used to consume the new-thread notification and leave the queue unseen.
  // The allocation override has reset its flag, so this poll can allocate normally. It must
  // retry discovery and deliver the record without another producer registering a queue.
  worker->poll_one();
  CHECK_EQ(sink->writes, 1u);
  worker->poll();

  // The MSVC debug STL allocates a container proxy for a vector as it is constructed, which is
  // noexcept, and an injected bad_alloc there terminates the process
  #if !defined(_ITERATOR_DEBUG_LEVEL) || (_ITERATOR_DEBUG_LEVEL == 0)
  // A poll lists the threads that have records to write, here two of them for the first time: the
  // second producer, which keeps one of its two records after a poll, and this thread. The list
  // has room for every thread that the backend has discovered, so that poll allocates nothing
  // and writes a record. At Backend::stop() a failure there left the queued records unwritten
  auto const log_from_this_thread = [logger]() { LOG_INFO(logger, "from the test thread"); };
  log_from_this_thread();
  worker->poll();

  std::thread second_producer(
    [logger]()
    {
      for (int i = 0; i < 2; ++i)
      {
        LOG_INFO(logger, "from the second producer");
      }
    });
  second_producer.join();
  worker->poll_one();
  CHECK_EQ(sink->writes, 3u);

  log_from_this_thread();
  fail_next_allocation = true;
  worker->poll_one();
  CHECK(fail_next_allocation);
  CHECK_EQ(sink->writes, 4u);
  fail_next_allocation = false;
  worker->poll();
  CHECK_EQ(sink->writes, 5u);

  // A logger that is removed is collected by a poll. When that fails to allocate, here the list
  // of the names of the removed loggers, a later poll must still remove the logger
  auto* removed_logger =
    quill::Frontend::create_or_get_logger("context_allocation_failure_logger_that_is_removed", sink);
  size_t const number_of_loggers = quill::Frontend::get_number_of_loggers();
  quill::Frontend::remove_logger(removed_logger);

  fail_next_allocation = true;
  worker->poll_one();
  CHECK_FALSE(fail_next_allocation);
  CHECK_EQ(quill::Frontend::get_number_of_loggers(), number_of_loggers);

  worker->poll_one();
  CHECK_EQ(quill::Frontend::get_number_of_loggers(), number_of_loggers - 1u);
  #endif

  // A thread that only creates its queue and exits leaves a retired context with nothing to read
  std::thread idle_thread([]() { quill::Frontend::preallocate(); });
  idle_thread.join();

  // Fail the discovery of that context. shutdown() does not poll again when
  // wait_for_queues_to_empty_before_exit is false, so it must cope with a context whose transit
  // buffer was never allocated.
  fail_next_allocation = true;
  worker->poll_one();
  CHECK_FALSE(fail_next_allocation);

  worker->shutdown();
  quill::Frontend::remove_logger(logger);
#endif
}
