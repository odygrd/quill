#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

#if !defined(QUILL_NO_EXCEPTIONS)
TEST_CASE("manual_backend_worker_error_notifier")
{
  static constexpr char const* filename = "manual_backend_worker_error_notifier.log";
  static std::string const logger_name = "manual_backend_worker_error_notifier_logger";

  std::atomic<bool> backend_ready{false};
  std::atomic<bool> stop_backend{false};
  std::atomic<size_t> error_notifier_invoked{0};

  std::thread backend_thread(
    [&]()
    {
      ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();

      BackendOptions backend_options;
      backend_options.error_notifier = [&error_notifier_invoked](std::string const&)
      { error_notifier_invoked.fetch_add(1, std::memory_order_relaxed); };

      manual_backend_worker->init(backend_options);
      backend_ready.store(true, std::memory_order_release);

      while (!stop_backend.load(std::memory_order_acquire))
      {
        manual_backend_worker->poll(std::chrono::microseconds{50});
        std::this_thread::yield();
      }

      manual_backend_worker->poll();
      manual_backend_worker->shutdown();
    });

  while (!backend_ready.load(std::memory_order_acquire))
  {
    std::this_thread::yield();
  }

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  LOG_INFO(logger, "valid message");
  LOG_INFO(logger, "Format {:>321.}", 321.3);

  auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
  while ((error_notifier_invoked.load(std::memory_order_relaxed) == 0) &&
         (std::chrono::steady_clock::now() < deadline))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }

  REQUIRE_EQ(error_notifier_invoked.load(std::memory_order_relaxed), 1);

  stop_backend.store(true, std::memory_order_release);
  backend_thread.join();
  Frontend::remove_logger(logger);

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " valid message"));

  testing::remove_file(filename);
}
#endif
