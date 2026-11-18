#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <chrono>
#include <cstddef>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>

// ThreadSanitizer supplies its own global new/delete definitions, which cannot be replaced.
#if !QUILL_HAS_FEATURE(thread_sanitizer) && !defined(__SANITIZE_THREAD__)
namespace
{
// Only the calling thread's next nothrow allocation fails; backend allocations are unaffected.
thread_local bool fail_next_nothrow_allocation{false};
} // namespace

// An unbounded queue allocates the node it grows into with nothrow new. This executable replaces
// both nothrow forms, as the node is over-aligned on most platforms, to simulate a single failed
// growth without exhausting memory. Other test executables use their normal allocator.
void* operator new(std::size_t size, std::align_val_t alignment, std::nothrow_t const&) noexcept
{
  if (fail_next_nothrow_allocation)
  {
    fail_next_nothrow_allocation = false;
    return nullptr;
  }

  return ::operator new(size, alignment);
}

void* operator new(std::size_t size, std::nothrow_t const&) noexcept
{
  if (fail_next_nothrow_allocation)
  {
    fail_next_nothrow_allocation = false;
    return nullptr;
  }

  return ::operator new(size);
}

namespace
{
struct SmallQueueFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedBlocking;
  static constexpr size_t initial_queue_capacity = 4096;
};
} // namespace
#endif

TEST_CASE("unbounded_blocking_queue_growth_failure")
{
#if QUILL_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
  return;
#else
  static constexpr char const* filename = "unbounded_blocking_queue_growth_failure.log";
  std::mutex notifications_mutex;
  std::vector<std::string> notifications;

  quill::BackendOptions backend_options;
  backend_options.error_notifier = [&notifications_mutex, &notifications](std::string const& message)
  {
    std::lock_guard<std::mutex> const lock{notifications_mutex};
    notifications.push_back(message);
  };

  quill::Backend::start(backend_options);

  using SmallQueueFrontend = quill::FrontendImpl<SmallQueueFrontendOptions>;
  quill::FileSinkConfig file_config;
  file_config.set_open_mode('w');
  auto sink = SmallQueueFrontend::create_or_get_sink<quill::FileSink>(filename, file_config);
  auto* logger =
    SmallQueueFrontend::create_or_get_logger("unbounded_blocking_queue_growth_failure_logger", sink,
                                             quill::PatternFormatterOptions{"%(message)%(mdc)"});

  // The first statement creates this thread's queue
  LOG_INFO(logger, "before the failed growth");
  logger->flush_log();

  // Larger than the whole queue, so it needs a bigger one, and allocating that one fails. The
  // backend cannot make room for this event by draining the current node.
  std::string const oversized(3 * SmallQueueFrontendOptions::initial_queue_capacity, 'x');

  fail_next_nothrow_allocation = true;
  LOG_INFO(logger, "oversized {}", oversized);
  REQUIRE_FALSE(fail_next_nothrow_allocation);

  // An oversized MDC update also drops without changing the thread context.
  fail_next_nothrow_allocation = true;
  logger->set_mdc("key", oversized);
  REQUIRE_FALSE(fail_next_nothrow_allocation);

  // Both were dropped, and the queue keeps working
  LOG_INFO(logger, "after the failed growth");
  logger->flush_log();

  auto const lines = quill::testing::file_contents(filename);
  REQUIRE_EQ(lines.size(), 2u);
  REQUIRE_EQ(lines.front(), "before the failed growth");
  REQUIRE_EQ(lines.back(), "after the failed growth");

  // The backend reports the drops while it is idle
  bool reported{false};

  for (int attempt = 0; (attempt < 500) && !reported; ++attempt)
  {
    {
      std::lock_guard<std::mutex> const lock{notifications_mutex};

      for (std::string const& notification : notifications)
      {
        if (notification.find("too large for the current queue") != std::string::npos)
        {
          reported = true;
        }
      }
    }

    if (!reported)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
  }

  REQUIRE(reported);

  #if !defined(QUILL_NO_EXCEPTIONS)
  // Blocking removal must report failure if its request (which includes the logger name) cannot fit.
  auto* long_named_logger = SmallQueueFrontend::create_or_get_logger(oversized, sink);
  fail_next_nothrow_allocation = true;
  REQUIRE_THROWS_AS(SmallQueueFrontend::remove_logger_blocking(long_named_logger), quill::QuillError);
  REQUIRE_FALSE(fail_next_nothrow_allocation);
  SmallQueueFrontend::remove_logger_blocking(long_named_logger);
  #endif

  SmallQueueFrontend::remove_logger_blocking(logger);
  sink.reset();
  quill::Backend::stop();
  quill::testing::remove_file(filename);
#endif
}
