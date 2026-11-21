#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <cstdio>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/**
 * Multithreaded backtrace logging stress test
 */
TEST_CASE("backtrace_multithreaded_stress")
{
  static constexpr char const* filename = "backtrace_multithreaded_stress.log";
  static std::string const logger_name = "backtrace_multithreaded_logger";

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

  logger->init_backtrace(10, LogLevel::Error);

  // minimum capacity
  BackendOptions backend_options;
  backend_options.transit_event_buffer_initial_capacity = 2;
  backend_options.transit_events_soft_limit = 4;

  Backend::start(backend_options);

  std::atomic<size_t> total_logged{0};
  size_t const messages_per_thread = 20000;
  size_t const num_threads = 4;

  auto thread_func = [&](size_t thread_id)
  {
    std::mt19937 rng{static_cast<uint32_t>(thread_id * 99999)};
    std::uniform_int_distribution<size_t> size_dist(10, 500);

    for (size_t i = 0; i < messages_per_thread; ++i)
    {
      // Create unique identifier
      std::string unique_id = "T" + std::to_string(thread_id) + "_BT" + std::to_string(i);

      // Vary string size
      size_t str_size = size_dist(rng);
      std::string msg(str_size, 'A' + (thread_id % 26));

      LOG_BACKTRACE(logger, "{}: {}", unique_id, msg);

      total_logged.fetch_add(1, std::memory_order_relaxed);

      // Trigger backtrace flush occasionally
      if (i % 30 == 0)
      {
        LOG_ERROR(logger, "T{} triggering backtrace flush at {}", thread_id, i);
      }
    }
  };

  std::vector<std::thread> threads;
  for (size_t t = 0; t < num_threads; ++t)
  {
    threads.emplace_back(thread_func, t);
  }

  for (auto& thread : threads)
  {
    thread.join();
  }

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);

  REQUIRE_GE(file_contents.size(),
             static_cast<size_t>(((messages_per_thread - 1) / 30 + 1) * num_threads * 1)); // min: 1 ERROR per flush
  REQUIRE_LE(file_contents.size(),
             static_cast<size_t>((((messages_per_thread - 1) / 30 + 1) * num_threads * (1 + 10)) // max per flush
                                 + (num_threads + 2) * (1 + 10)));

  testing::remove_file(filename);
}
