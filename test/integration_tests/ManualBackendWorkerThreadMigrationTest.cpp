#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace quill;

TEST_CASE("manual_backend_worker_allows_serialized_thread_migration")
{
  static constexpr char const* filename = "manual_backend_worker_thread_migration.log";
  static std::string const logger_name = "manual_backend_worker_thread_migration_logger";

  testing::remove_file(filename);

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  BackendOptions backend_options;
  manual_backend_worker->init(backend_options, true);

  REQUIRE_EQ(Backend::get_thread_id(), 0u);

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

  std::atomic<uint32_t> next_poll_thread{0};
  std::atomic<bool> stop_polling{false};
  std::atomic<bool> first_thread_has_polled{false};
  std::atomic<bool> second_thread_has_polled{false};
  std::atomic<bool> observed_nonzero_backend_thread_id{false};

  auto const poll_worker = [&](uint32_t const poll_thread_index, std::atomic<bool>& thread_has_polled)
  {
    while (!stop_polling.load(std::memory_order_acquire))
    {
      if (next_poll_thread.load(std::memory_order_acquire) != poll_thread_index)
      {
        std::this_thread::yield();
        continue;
      }

      if (Backend::get_thread_id() != 0u)
      {
        observed_nonzero_backend_thread_id.store(true, std::memory_order_release);
      }

      manual_backend_worker->poll();
      thread_has_polled.store(true, std::memory_order_release);
      next_poll_thread.store(1u - poll_thread_index, std::memory_order_release);
    }
  };

  std::thread first_poll_thread([&]() { poll_worker(0u, first_thread_has_polled); });
  std::thread second_poll_thread([&]() { poll_worker(1u, second_thread_has_polled); });

  while (!first_thread_has_polled.load(std::memory_order_acquire) ||
         !second_thread_has_polled.load(std::memory_order_acquire))
  {
    std::this_thread::yield();
  }

  for (uint32_t i = 0; i < 10u; ++i)
  {
    LOG_INFO(logger, "migratable backend message {}", i);
  }

  // This producer is also the thread that called init(). It must no longer be marked as the
  // backend thread, and the flush is completed by the two alternating polling threads above.
  logger->flush_log();
  stop_polling.store(true, std::memory_order_release);

  first_poll_thread.join();
  second_poll_thread.join();

  std::thread shutdown_thread([&]() { manual_backend_worker->shutdown(); });
  shutdown_thread.join();

  REQUIRE_FALSE(observed_nonzero_backend_thread_id.load(std::memory_order_acquire));
  REQUIRE_EQ(Backend::get_thread_id(), 0u);

  Frontend::remove_logger(logger);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 10u);
  for (uint32_t i = 0; i < 10u; ++i)
  {
    REQUIRE(testing::file_contains(file_contents, "migratable backend message " + std::to_string(i)));
  }

  testing::remove_file(filename);
}
