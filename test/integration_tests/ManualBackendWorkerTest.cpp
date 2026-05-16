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
#include <string_view>
#include <thread>
#include <vector>

using namespace quill;

TEST_CASE("manual_backend_worker")
{
  static constexpr char const* filename = "manual_backend_worker.log";
  static std::string const logger_name = "manual_backend_worker_logger";
  static constexpr size_t number_of_messages = 16;

  std::atomic<bool> backend_ready{false};
  std::atomic<bool> stop_backend{false};

  std::thread backend_thread(
    [&]()
    {
      ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();

      BackendOptions backend_options;
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

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(Backend::acquire_manual_backend_worker(), QuillError);
#endif

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

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "manual backend message {}", i);
  }

  stop_backend.store(true, std::memory_order_release);
  backend_thread.join();
  Frontend::remove_logger(logger);

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    REQUIRE(quill::testing::file_contains(
      file_contents, logger_name + " manual backend message " + std::to_string(i)));
  }

  testing::remove_file(filename);
}
