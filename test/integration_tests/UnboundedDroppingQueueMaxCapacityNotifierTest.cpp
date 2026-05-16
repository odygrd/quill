#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <string>
#include <vector>

using namespace quill;

struct UnboundedDroppingFrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedDropping;
  static constexpr size_t initial_queue_capacity = 1024;
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr size_t unbounded_queue_max_capacity = 2048;
  static constexpr quill::HugePagesPolicy huge_pages_policy = quill::HugePagesPolicy::Never;
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

  backend_options.sleep_duration = std::chrono::seconds{10};
  Backend::start(backend_options);

  auto file_sink = UnboundedDroppingFrontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  UnboundedDroppingLogger* logger =
    UnboundedDroppingFrontend::create_or_get_logger(logger_name, std::move(file_sink));

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string payload(256, static_cast<char>('A' + (i % 26)));
    LOG_INFO(logger, "message {} {}", i, payload);
  }

  Backend::notify();

  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  } while (testing::file_contents(filename).empty());

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

  testing::remove_file(filename);
}
