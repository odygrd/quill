#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/core/MacroMetadata.h"
#include "quill/sinks/FileSink.h"

#include <string>
#include <vector>

using namespace quill;

struct UnboundedDroppingFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedDropping;
  static constexpr size_t initial_queue_capacity = 1024;
  static constexpr size_t unbounded_queue_max_capacity = 2048;
};

using UnboundedDroppingFrontend = FrontendImpl<UnboundedDroppingFrontendOptions>;
using UnboundedDroppingLogger = LoggerImpl<UnboundedDroppingFrontendOptions>;

TEST_CASE("unbounded_dropping_queue_max_capacity_notifier")
{
  static constexpr char const* filename = "unbounded_dropping_queue_max_capacity_notifier.log";
  static std::string const logger_name = "logger";
  static constexpr size_t number_of_messages = 40u;

  BackendOptions backend_options;
  std::vector<std::string> notifications;
  backend_options.error_notifier = [&notifications](std::string const& error_message)
  { notifications.push_back(error_message); };

  auto file_sink = UnboundedDroppingFrontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  UnboundedDroppingLogger* logger = UnboundedDroppingFrontend::create_or_get_logger(
    logger_name, std::move(file_sink), PatternFormatterOptions{}, ClockSourceType::System);

  static constexpr MacroMetadata metadata{
    "", "", "message {} {}", nullptr, LogLevel::Info, MacroMetadata::Event::Log};
  std::vector<std::string> expected_messages;

  // Fill before starting the backend so growth and dropping are deterministic.
  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string payload(256, static_cast<char>('A' + (i % 26)));
    bool const accepted = logger->log_statement<false>(&metadata, i, payload);
    if (accepted)
    {
      expected_messages.push_back("message " + std::to_string(i) + " " + payload);
    }
  }

  Backend::start(backend_options);
  Backend::notify();

  logger->flush_log();
  UnboundedDroppingFrontend::remove_logger(logger);
  Backend::notify();
  Backend::stop();

  bool saw_reallocation = false;
  bool saw_max_capacity_drop = false;
  for (std::string const& notification : notifications)
  {
    if (notification.find("Allocated a new SPSC queue") != std::string::npos)
    {
      saw_reallocation = true;
    }

    if (notification.find("maximum configured unbounded queue capacity") != std::string::npos &&
        notification.find("dropped") != std::string::npos)
    {
      saw_max_capacity_drop = true;
    }
  }

  REQUIRE(saw_reallocation);
  REQUIRE(saw_max_capacity_drop);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), expected_messages.size());
  for (std::string const& message : expected_messages)
  {
    REQUIRE(testing::file_contains(file_contents, message));
  }

  testing::remove_file(filename);
}
