#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

using namespace quill;

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced
#if !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
// The bytes allocated through operator new and not yet deleted
std::atomic<size_t> live_bytes{0};

// Keeps the size of each allocation in front of it, aligned like any allocation
constexpr size_t allocation_header_size{alignof(std::max_align_t)};

// The counting lives in functions that are not inlined. GCC 16 at -O3 otherwise sees the header
// read and the free() through the inlined operators, and reports an out of bounds access and a
// mismatched deallocation. The operators themselves take no attribute, which GCC 8 rejects.
QUILL_NOINLINE void* allocate_counted(std::size_t size)
{
  void* const memory = std::malloc(size + allocation_header_size);

  if (!memory)
  {
    std::abort();
  }

  *static_cast<size_t*>(memory) = size;
  live_bytes.fetch_add(size, std::memory_order_relaxed);
  return static_cast<char*>(memory) + allocation_header_size;
}

QUILL_NOINLINE void deallocate_counted(void* memory) noexcept
{
  if (!memory)
  {
    return;
  }

  void* const block = static_cast<char*>(memory) - allocation_header_size;
  live_bytes.fetch_sub(*static_cast<size_t*>(block), std::memory_order_relaxed);
  std::free(block);
}
} // namespace

void* operator new(std::size_t size) { return allocate_counted(size); }

void operator delete(void* memory) noexcept { deallocate_counted(memory); }

void operator delete(void* memory, std::size_t) noexcept { deallocate_counted(memory); }
#endif

/** Discards the records: only the memory of the backend is measured */
class BackendStopReleasesScratchMemorySink final : public Sink
{
public:
  void write_log(LogRecord const&) override {}
  void flush_sink(SinkFlushReason) noexcept override {}
};

/***/
TEST_CASE("backend_stop_releases_scratch_memory")
{
#if QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#else
  Backend::start();

  Logger* logger =
    Frontend::create_or_get_logger("backend_stop_releases_scratch_memory_logger",
                                   Frontend::create_or_get_sink<BackendStopReleasesScratchMemorySink>(
                                     "backend_stop_releases_scratch_memory_sink"),
                                   PatternFormatterOptions{"%(message)"});

  std::string const mdc_text(32u * 1024u, 'm');

  std::vector<MetricLabel> labels;
  for (size_t i = 0; i < 256; ++i)
  {
    labels.emplace_back("label_" + std::to_string(i), "value");
  }

  // Creates the queue, the state of this thread and the registered metric before the baseline.
  // The backend copies the labels into its scratch storage
  LOG_INFO(logger, "before the baseline");
  REQUIRE(logger->publish_dynamic_metric("backend_stop_releases_scratch_memory_metric", labels, 1.0));
  logger->flush_log();

  size_t const baseline_bytes = live_bytes.load();

  // The backend copies the field into its scratch storage, and then the erased key
  logger->set_mdc("request", mdc_text);
  logger->flush_log();
  logger->erase_mdc(mdc_text);
  logger->clear_mdc();
  logger->flush_log();

  Backend::stop();

  // The MDC copies are freed, and so are the 256 label slots that were already in the baseline
  size_t const after_stop_bytes = live_bytes.load();
  REQUIRE_LE(after_stop_bytes + labels.size() * sizeof(MetricLabel) / 2u, baseline_bytes);
#endif
}
