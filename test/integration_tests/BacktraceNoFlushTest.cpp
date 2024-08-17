#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("backtrace_no_flush")
{
  static constexpr char const* filename = "backtrace_no_flush.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  Frontend::preallocate();

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

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  // Enable backtrace for 2 messages for warning
  logger->init_backtrace(2, LogLevel::Error);

  // try with LOG_WARNING and expect no flush
  LOG_INFO(logger, "Before backtrace log retry");
  for (size_t i = 0; i < 12; ++i)
  {
    LOG_BACKTRACE(logger, "Backtrace log {}", i);
  }
  LOG_WARNING(logger, "After backtrace log retry");

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 2);

  std::string expected_string_1 =
    "LOG_INFO      " + logger_name + "       Before backtrace log retry";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string_1));

  std::string expected_string_2 =
    "LOG_WARNING   " + logger_name + "       After backtrace log retry";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string_2));

  testing::remove_file(filename);
}