#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/NullSink.h"

#include <array>
#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/***/
TEST_CASE("backend_transit_buffer_hard_limit")
{
  static constexpr size_t number_of_messages = 500u;
  static constexpr size_t number_of_threads = 10;
  static constexpr char const* filename = "backend_transit_buffer_hard_limit.log";
  static std::string const logger_name_prefix = "logger_";

  // Start the backend thread
  BackendOptions backend_options;
  backend_options.transit_events_hard_limit = 0;
  backend_options.transit_events_soft_limit = 0;
  backend_options.transit_event_buffer_initial_capacity = 0;
  Backend::start(backend_options);

  std::vector<std::thread> threads;

  for (size_t i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back(
      [i]() mutable
      {
        // Set writing logging to a file
        auto file_sink = Frontend::create_or_get_sink<FileSink>(
          filename,
          []()
          {
            FileSinkConfig cfg;
            cfg.set_open_mode('w');
            return cfg;
          }(),
          FileEventNotifier{});

        Logger* logger =
          Frontend::create_or_get_logger(logger_name_prefix + std::to_string(i), std::move(file_sink));

        for (size_t j = 0; j < number_of_messages; ++j)
        {
          LOG_INFO(logger, "Hello from thread {thread_index} this is message {message_num}", i, j);
        }
      });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

  // flush all log and remove all loggers
  for (Logger* logger : Frontend::get_all_loggers())
  {
    logger->flush_log();
    Frontend::remove_logger(logger);
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), number_of_messages * number_of_threads);

  for (size_t i = 0; i < number_of_threads; ++i)
  {
    // for each thread
    for (size_t j = 0; j < number_of_messages; ++j)
    {
      std::string expected_string = logger_name_prefix + std::to_string(i) +
        "     Hello from thread " + std::to_string(i) + " this is message " + std::to_string(j);

      REQUIRE(testing::file_contains(file_contents, expected_string));
    }
  }

  testing::remove_file(filename);

  // Direct boundary regression: with a per-thread hard limit of one, the reader must not pop
  // from a producer whose transit buffer is already full. The old do/while loop always accepted
  // one extra event and expanded that buffer beyond the configured limit.
  BackendOptions manual_options;
  manual_options.transit_event_buffer_initial_capacity = 1;
  manual_options.transit_events_soft_limit = 1;
  manual_options.transit_events_hard_limit = 1;

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(manual_options);

  auto null_sink =
    Frontend::create_sink<NullSink>("backend_transit_buffer_hard_limit_boundary_null_sink");
  Logger* boundary_logger =
    Frontend::create_logger("backend_transit_buffer_hard_limit_boundary_logger", std::move(null_sink));

  std::array<std::atomic<detail::ThreadContext*>, 2> producer_contexts{};
  for (auto& context : producer_contexts)
  {
    context.store(nullptr, std::memory_order_relaxed);
  }

  std::atomic<bool> release_producers{false};
  std::array<std::thread, 2> producers;
  for (size_t i = 0; i < producers.size(); ++i)
  {
    producers[i] = std::thread(
      [i, boundary_logger, &producer_contexts, &release_producers]()
      {
        LOG_INFO(boundary_logger, "hard-limit boundary {} first", i);
        LOG_INFO(boundary_logger, "hard-limit boundary {} second", i);

        producer_contexts[i].store(detail::get_local_thread_context<FrontendOptions>(), std::memory_order_release);

        while (!release_producers.load(std::memory_order_acquire))
        {
          std::this_thread::yield();
        }
      });
  }

  for (auto& context : producer_contexts)
  {
    while (context.load(std::memory_order_acquire) == nullptr)
    {
      std::this_thread::yield();
    }
  }

  manual_backend_worker->poll_one();
  manual_backend_worker->poll_one();

  size_t empty_producer_queues{0};
  for (auto& context : producer_contexts)
  {
    detail::ThreadContext* thread_context = context.load(std::memory_order_acquire);
    if (thread_context->get_spsc_queue<FrontendOptions::queue_type>().empty())
    {
      ++empty_producer_queues;
    }
  }

  release_producers.store(true, std::memory_order_release);
  for (auto& producer : producers)
  {
    producer.join();
  }

  manual_backend_worker->poll();
  Frontend::remove_logger(boundary_logger);
  manual_backend_worker->poll();
  manual_backend_worker->shutdown();

  // Exactly one empty queue distinguishes the fixed while-loop from the old unconditional first
  // read, which emptied both queues and grew the already-full transit buffer.
  REQUIRE_EQ(empty_producer_queues, 1u);
}
