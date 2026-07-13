#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

struct BackendCallbackFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedBlocking;
  static constexpr size_t initial_queue_capacity = 1024;
};

using BackendCallbackFrontend = FrontendImpl<BackendCallbackFrontendOptions>;
using BackendCallbackLogger = LoggerImpl<BackendCallbackFrontendOptions>;

/***/
TEST_CASE("backend_immediate_flush_from_backend_thread")
{
  static constexpr char const* filename = "backend_immediate_flush_from_backend_thread.log";
  static std::string const logger_name = "backend_immediate_flush_from_backend_thread_logger";

  testing::remove_file(filename);

  std::atomic<BackendCallbackLogger*> logger_for_hook{nullptr};
  std::atomic<bool> hook_started{false};
  std::atomic<bool> hook_done{false};
#if !defined(QUILL_NO_EXCEPTIONS)
  std::atomic<bool> explicit_flush_rejected{false};
  std::atomic<bool> backend_stop_rejected{false};
  std::atomic<bool> blocking_remove_rejected{false};
#endif

  BackendOptions backend_options;
  backend_options.error_notifier = {};
  backend_options.backend_worker_on_poll_begin = [&logger_for_hook, &hook_started, &hook_done
#if !defined(QUILL_NO_EXCEPTIONS)
                                                  ,
                                                  &explicit_flush_rejected, &backend_stop_rejected, &blocking_remove_rejected
#endif
  ]()
  {
    BackendCallbackLogger* logger = logger_for_hook.load(std::memory_order_acquire);
    if (logger && !hook_started.exchange(true, std::memory_order_acq_rel))
    {
      // Immediate flush is enabled - logging from the backend thread must not deadlock,
      // so the implicit flush is silently skipped.
      for (uint32_t i = 0; i < 1000; ++i)
      {
        LOG_INFO(logger, "backend hook log with immediate flush enabled {}", i);
      }

      // These operations normally retry failed control-event enqueues. Once the backend has filled
      // its own queue, they must drop instead of waiting for themselves to consume it.
      logger->set_mdc("backend_callback", "value");
      logger->erase_mdc("backend_callback");
      logger->clear_mdc();
      logger->init_backtrace(1);
      logger->flush_backtrace();

#if !defined(QUILL_NO_EXCEPTIONS)
      // An explicit flush_log() from the backend thread should surface as a QuillError
      // rather than self-deadlock.
      try
      {
        logger->flush_log();
      }
      catch (QuillError const&)
      {
        explicit_flush_rejected.store(true, std::memory_order_release);
      }

      // Backend::stop() would try to join the current thread if called here.
      try
      {
        Backend::stop();
      }
      catch (QuillError const&)
      {
        backend_stop_rejected.store(true, std::memory_order_release);
      }

      // Blocking logger removal also waits for the backend to process a control event.
      try
      {
        BackendCallbackFrontend::remove_logger_blocking(logger);
      }
      catch (QuillError const&)
      {
        blocking_remove_rejected.store(true, std::memory_order_release);
      }
#endif

      // Set last; the main thread waits on this so it never observes a half-finished hook.
      hook_done.store(true, std::memory_order_release);
    }
  };

  Backend::start(backend_options);

  auto file_sink = BackendCallbackFrontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  BackendCallbackLogger* logger =
    BackendCallbackFrontend::create_or_get_logger(logger_name, std::move(file_sink));
  logger->set_immediate_flush(1);
  logger_for_hook.store(logger, std::memory_order_release);

  for (uint32_t i = 0; (i < 1000) && !hook_done.load(std::memory_order_acquire); ++i)
  {
    Backend::notify();
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  REQUIRE(hook_done.load(std::memory_order_acquire));
#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE(explicit_flush_rejected.load(std::memory_order_acquire));
  REQUIRE(backend_stop_rejected.load(std::memory_order_acquire));
  REQUIRE(blocking_remove_rejected.load(std::memory_order_acquire));
#endif

  LOG_INFO(logger, "frontend log after backend hook");
  logger->flush_log();

  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE(
    quill::testing::file_contains(file_contents, "backend hook log with immediate flush enabled 0"));
  REQUIRE(file_contents.size() < 1001u);
  REQUIRE(quill::testing::file_contains(file_contents, "frontend log after backend hook"));

  testing::remove_file(filename);
}
