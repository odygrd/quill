#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <string>
#include <vector>

using namespace quill;

// Define custom Frontend Options
struct CustomFrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
  static constexpr uint32_t initial_queue_capacity = 131'072;
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr bool huge_pages_enabled = false;
};

using CustomFrontend = FrontendImpl<CustomFrontendOptions>;
using CustomLogger = LoggerImpl<CustomFrontendOptions>;

TEST_CASE("bounded_dropping_queue")
{
  static constexpr char const* filename = "bounded_dropping_queue.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  auto file_sink = CustomFrontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  CustomLogger* logger = CustomFrontend::create_or_get_logger(logger_name, std::move(file_sink));

  for (int i = 0; i < 5000; ++i)
  {
    LOG_INFO(logger, "Log something to fulfill the bound queue {}", i);
    LOG_WARNING(logger, "Log something to fulfill the bound queue {}", i);
    LOG_ERROR(logger, "Log something to fulfill the bound queue {}", i);
  }

  logger->flush_log(0);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check the first messages as we can't if some were dropped
  std::vector<std::string> const file_contents = testing::file_contents(filename);

  for (int i = 0; i < 100; ++i)
  {
    std::string expected_string_1 = "LOG_INFO      " + logger_name +
      "       Log something to fulfill the bound queue " + std::to_string(i);

    std::string expected_string_2 = "LOG_WARNING   " + logger_name +
      "       Log something to fulfill the bound queue " + std::to_string(i);

    std::string expected_string_3 = "LOG_ERROR     " + logger_name +
      "       Log something to fulfill the bound queue " + std::to_string(i);

    REQUIRE(testing::file_contains(file_contents, expected_string_1));
    REQUIRE(testing::file_contains(file_contents, expected_string_2));
    REQUIRE(testing::file_contains(file_contents, expected_string_3));
  }

  testing::remove_file(filename);
}
