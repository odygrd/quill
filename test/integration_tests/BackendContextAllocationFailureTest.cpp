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
}

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
    void write_log(quill::MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                   std::string const&, std::string_view, quill::LogLevel, std::string_view,
                   std::string_view, std::vector<std::pair<std::string, std::string>> const*,
                   std::string_view, std::string_view) override
    {
      ++writes;
    }
    void flush_sink() override {}
    size_t writes{0};
  };

  size_t errors{0};
  quill::BackendOptions options;
  options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  options.error_notifier = [&errors](std::string const&) { ++errors; };
  auto* worker = quill::Backend::acquire_manual_backend_worker();
  worker->init(options);

  auto sink = std::static_pointer_cast<CountingSink>(
    quill::Frontend::create_or_get_sink<CountingSink>("context_allocation_failure_sink"));
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
  worker->shutdown();
  quill::Frontend::remove_logger(logger);
#endif
}
