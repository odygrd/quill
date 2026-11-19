#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"
#include "quill/std/Vector.h"

#include <atomic>
#include <cstddef>
#include <new>
#include <vector>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
#if !defined(QUILL_NO_EXCEPTIONS) && !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
// Only the calling thread's next allocation fails; backend allocations are unaffected.
thread_local bool fail_next_allocation{false};
} // namespace

// This executable replaces global operator new[] to simulate a single allocation failure
// without exhausting memory. Other test executables use their normal allocator.
void* operator new[](std::size_t size)
{
  if (fail_next_allocation)
  {
    fail_next_allocation = false;
    throw std::bad_alloc{};
  }

  return ::operator new[](size, std::align_val_t{alignof(std::max_align_t)});
}

void operator delete[](void* memory) noexcept
{
  ::operator delete[](memory, std::align_val_t{alignof(std::max_align_t)});
}
void operator delete[](void* memory, std::size_t) noexcept
{
  ::operator delete[](memory, std::align_val_t{alignof(std::max_align_t)});
}
#endif

TEST_CASE("size_cache_allocation_failure")
{
#if defined(QUILL_NO_EXCEPTIONS) || QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#else
  class CountingSink : public quill::Sink
  {
  public:
    void write_log(quill::LogRecord const& /* record */) override { writes.fetch_add(1); }
    void flush_sink(quill::SinkFlushReason) override {}
    std::atomic<size_t> writes{0};
  };

  quill::Backend::start();

  auto sink =
    quill::Frontend::create_or_get_sink<CountingSink>("size_cache_allocation_failure_sink");
  auto* logger = quill::Frontend::create_or_get_logger("size_cache_allocation_failure_logger", sink);

  // The first statement creates this thread's queue and size cache
  LOG_INFO(logger, "before the allocation failure");
  logger->flush_log();
  REQUIRE_EQ(sink->writes.load(), 1u);

  // The size cache keeps 12 string lengths inline, so the thirteenth C string makes it allocate.
  // Sizing used to be noexcept, and that failed allocation called std::terminate.
  std::vector<char const*> const c_strings(13, "c_string");
  bool caught_bad_alloc{false};

  fail_next_allocation = true;

  try
  {
    LOG_INFO(logger, "strings {}", c_strings);
  }
  catch (std::bad_alloc const&)
  {
    caught_bad_alloc = true;
  }

  fail_next_allocation = false;
  REQUIRE(caught_bad_alloc);

  // Nothing was enqueued for the failed statement, and the same statement is logged afterwards
  LOG_INFO(logger, "strings {}", c_strings);
  logger->flush_log();
  REQUIRE_EQ(sink->writes.load(), 2u);

  quill::Backend::stop();
#endif
}
