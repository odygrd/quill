#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

namespace
{
struct CustomFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
  static constexpr size_t initial_queue_capacity = 1024;
};

using CustomFrontend = FrontendImpl<CustomFrontendOptions>;
using CustomLogger = LoggerImpl<CustomFrontendOptions>;

void log_runtime_metadata_message(CustomLogger* logger, size_t index, std::string const& payload)
{
  uint32_t const line_number = 4000u + static_cast<uint32_t>(index);

  switch (index % 3)
  {
  case 0:
    LOG_RUNTIME_METADATA(logger, quill::LogLevel::Info,
                         "RuntimeMetadataDroppingQueueNotifierTest.cpp", line_number, "deep_event",
                         "drop deep {} {}", index, payload);
    break;

  case 1:
    LOG_RUNTIME_METADATA_HYBRID(logger, quill::LogLevel::Info,
                                "RuntimeMetadataDroppingQueueNotifierTest.cpp", line_number,
                                "hybrid_event", "", "drop hybrid {} {}", index, payload);
    break;

  default:
    LOG_RUNTIME_METADATA_SHALLOW(logger, quill::LogLevel::Info,
                                 "RuntimeMetadataDroppingQueueNotifierTest.cpp", line_number,
                                 "shallow_event", "", "drop shallow {} {}", index, payload);
    break;
  }
}

void enqueue_runtime_metadata_message_until_success(CustomLogger* logger)
{
  static constexpr MacroMetadata macro_metadata{
    "[placeholder]", "[placeholder]", "[placeholder]",
    nullptr,         LogLevel::None,  MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy};

  while (!logger->log_statement_runtime_metadata<false>(
    &macro_metadata, "final runtime metadata message {}",
    "RuntimeMetadataDroppingQueueNotifierTest.cpp", "final_event", "", 9001u, LogLevel::Info, 42))
  {
    Backend::notify();
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
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

TEST_CASE("runtime_metadata_dropping_queue_notifier")
{
  static constexpr char const* filename = "runtime_metadata_dropping_queue_notifier.log";
  static std::string const logger_name = "runtime_metadata_dropping_logger";
  static constexpr size_t number_of_messages = 30u;

  BackendOptions backend_options;

  std::string dropped_message;
  backend_options.error_notifier = [&dropped_message](std::string const& error_message)
  {
    if (error_message.find("Dropped ") != std::string::npos)
    {
      dropped_message = error_message;
    }
  };

  backend_options.sleep_duration = std::chrono::seconds{10};
  Backend::start(backend_options);

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

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string payload(96u + i, static_cast<char>('A' + (i % 26)));
    log_runtime_metadata_message(logger, i, payload);
  }

  Backend::notify();

  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  } while (testing::file_contents(filename).empty());

  enqueue_runtime_metadata_message_until_success(logger);

  Backend::notify();
  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::notify();
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);

  REQUIRE(testing::file_contains(file_contents, "drop deep 0"));
  REQUIRE(testing::file_contains(file_contents, "drop hybrid 1"));
  REQUIRE(testing::file_contains(file_contents, "drop shallow 2"));
  REQUIRE(testing::file_contains(file_contents, "final runtime metadata message 42"));

  uint64_t const dropped_events = extract_count(dropped_message, "Dropped ", " events from thread ");

  REQUIRE_GE(dropped_events, 1u);
  REQUIRE_LE(dropped_events, number_of_messages);

  testing::remove_file(filename);
}
