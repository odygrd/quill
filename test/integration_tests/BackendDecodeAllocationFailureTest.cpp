#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"
#include "quill/std/Vector.h"

#include <cstddef>
#include <new>
#include <string>
#include <thread>
#include <vector>

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

TEST_CASE("backend_decode_allocation_failure_retries_with_fresh_arguments")
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
    void write_log(quill::MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                   std::string const&, std::string_view, quill::LogLevel, std::string_view,
                   std::string_view, std::vector<std::pair<std::string, std::string>> const*,
                   std::string_view log_message, std::string_view) override
    {
      messages.emplace_back(log_message);
    }
    void flush_sink() override {}
    std::vector<std::string> messages;
  };

  size_t errors{0};
  quill::BackendOptions options;
  options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  options.error_notifier = [&errors](std::string const&) { ++errors; };
  auto* worker = quill::Backend::acquire_manual_backend_worker();
  worker->init(options);

  auto sink = std::static_pointer_cast<CapturingSink>(
    quill::Frontend::create_or_get_sink<CapturingSink>("decode_allocation_failure_sink"));
  auto* logger = quill::Frontend::create_or_get_logger("decode_allocation_failure_logger", sink);

  // Discover this thread's context and grow the backend's scratch buffers
  LOG_INFO(logger, "warmup {} {}", 1, 2);
  worker->poll();
  sink->messages.clear();

  // poll() discovering a new frontend thread must report an allocation failure instead of
  // letting it escape to the caller
  std::thread producer([logger]() { LOG_INFO(logger, "from another thread"); });
  producer.join();
  fail_next_allocation = true;
  worker->poll();
  CHECK_EQ(errors, 1u);
  worker->poll();
  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0], "from another thread");
  sink->messages.clear();

  // The vector argument allocates while decoding; after the failure the record is decoded again
  // and must not be formatted with the int left over from the failed attempt
  std::vector<int> const values{7, 8, 9};
  LOG_INFO(logger, "id={} values={}", 42, values);
  fail_next_allocation = true;
  worker->poll_one(); // reported as the same bad_alloc, so the notifier suppresses the repeat
  worker->poll();

  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0], "id=42 values=[7, 8, 9]");

  worker->shutdown();
  quill::Frontend::remove_logger(logger);
#endif
}
