#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

namespace
{
struct CustomFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedBlocking;
  static constexpr size_t initial_queue_capacity = 1024;
};

using CustomFrontend = FrontendImpl<CustomFrontendOptions>;
using CustomLogger = LoggerImpl<CustomFrontendOptions>;

void log_runtime_metadata_message(CustomLogger* logger, size_t index, std::string const& payload)
{
  uint32_t const line_number = 5000u + static_cast<uint32_t>(index);

  switch (index % 3)
  {
  case 0:
    LOG_RUNTIME_METADATA(logger, quill::LogLevel::Info,
                         "RuntimeMetadataBlockingQueueNotifierTest.cpp", line_number, "deep_event",
                         "block deep {} {}", index, payload);
    break;

  case 1:
    LOG_RUNTIME_METADATA_HYBRID(logger, quill::LogLevel::Info,
                                "RuntimeMetadataBlockingQueueNotifierTest.cpp", line_number,
                                "hybrid_event", "", "block hybrid {} {}", index, payload);
    break;

  default:
    LOG_RUNTIME_METADATA_SHALLOW(logger, quill::LogLevel::Info,
                                 "RuntimeMetadataBlockingQueueNotifierTest.cpp", line_number,
                                 "shallow_event", "", "block shallow {} {}", index, payload);
    break;
  }
}

uint64_t extract_count(std::string const& notification, std::string const& search_start, std::string const& search_end)
{
  std::size_t start_pos = notification.find(search_start);
  std::size_t end_pos = notification.find(search_end);

  if ((start_pos == std::string::npos) || (end_pos == std::string::npos))
  {
    return 0;
  }

  start_pos += search_start.length();
  return std::stoull(notification.substr(start_pos, end_pos - start_pos));
}
} // namespace

TEST_CASE("runtime_metadata_blocking_queue_notifier")
{
  static constexpr char const* filename = "runtime_metadata_blocking_queue_notifier.log";
  static std::string const logger_name = "runtime_metadata_blocking_logger";
  static constexpr size_t number_of_messages = 40u;

  BackendOptions backend_options;

  std::atomic<uint64_t> blocking_events{0};

  backend_options.error_notifier = [&blocking_events](std::string const& error_message)
  {
    if (error_message.find("blocking occurrences") != std::string::npos)
    {
      uint64_t count =
        extract_count(error_message, "Experienced ", " blocking occurrences on thread ");

      if (count == 0)
      {
        count = extract_count(error_message, "experienced ", " blocking occurrences on thread ");
      }

      if (count != 0)
      {
        blocking_events.store(count, std::memory_order_release);
      }
    }
  };

  backend_options.sleep_duration = std::chrono::seconds{10};

  auto file_sink = CustomFrontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  CustomLogger* logger = CustomFrontend::create_or_get_logger(
    logger_name, std::move(file_sink),
    PatternFormatterOptions{
      "%(short_source_location) %(caller_function) LOG_%(log_level:<9) %(logger:<12) %(message)"});

  // The backend is intentionally not started yet. If it were running and actively polling, it
  // could drain the small queue in lockstep with the producer and no write would ever find the
  // queue full. With nothing consuming, the producer is guaranteed to fill the bounded queue
  // and block, since the messages do not all fit in it.
  std::atomic<detail::ThreadContext*> producer_thread_context{nullptr};
  std::atomic<bool> producer_finished{false};
  std::atomic<bool> allow_producer_exit{false};
  std::thread producer(
    [logger, &producer_thread_context, &producer_finished, &allow_producer_exit]()
    {
      producer_thread_context.store(detail::get_local_thread_context<CustomFrontendOptions>(),
                                    std::memory_order_release);

      for (size_t i = 0; i < number_of_messages; ++i)
      {
        std::string payload(96u + i, static_cast<char>('a' + (i % 26)));
        log_runtime_metadata_message(logger, i, payload);
      }

      producer_finished.store(true, std::memory_order_release);

      while (!allow_producer_exit.load(std::memory_order_acquire))
      {
        std::this_thread::sleep_for(std::chrono::microseconds{100});
      }
    });

  detail::ThreadContext* thread_context{nullptr};
  while (!(thread_context = producer_thread_context.load(std::memory_order_acquire)))
  {
    std::this_thread::sleep_for(std::chrono::microseconds{100});
  }

  // Wait until the producer has blocked at least once before starting the backend. Reading the
  // failure counter resets it; the harvested count is restored after the producer has finished
  // logging, while the producer thread is still alive and its ThreadContext cannot be cleaned up.
  size_t harvested_failures{0};
  while (harvested_failures == 0)
  {
    harvested_failures = thread_context->get_and_reset_failure_counter();
    std::this_thread::sleep_for(std::chrono::microseconds{100});
  }

  Backend::start(backend_options);

  while (!producer_finished.load(std::memory_order_acquire))
  {
    Backend::notify();
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }

  for (size_t i = 0; i < harvested_failures; ++i)
  {
    thread_context->increment_failure_counter();
  }

  Backend::notify();

  while (blocking_events.load(std::memory_order_acquire) == 0)
  {
    Backend::notify();
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }

  allow_producer_exit.store(true, std::memory_order_release);
  producer.join();

  LOG_RUNTIME_METADATA_SHALLOW(logger, quill::LogLevel::Info,
                               "RuntimeMetadataBlockingQueueNotifierTest.cpp", 9101u, "final_event",
                               "", "final blocking runtime metadata {}", 42);

  Backend::notify();
  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::notify();
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), number_of_messages + 1u);
  REQUIRE(testing::file_contains(file_contents, "block deep 0"));
  REQUIRE(testing::file_contains(file_contents, "block hybrid 1"));
  REQUIRE(testing::file_contains(file_contents, "block shallow 2"));
  REQUIRE(testing::file_contains(file_contents, "final blocking runtime metadata 42"));

  uint64_t const notified_blocking_events = blocking_events.load(std::memory_order_acquire);
  REQUIRE_GE(notified_blocking_events, 1u);
  REQUIRE_LE(notified_blocking_events, number_of_messages);

  testing::remove_file(filename);
}
