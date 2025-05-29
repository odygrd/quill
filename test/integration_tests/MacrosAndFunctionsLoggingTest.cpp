#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("macros_and_functions_logging_test")
{
  static constexpr size_t number_of_messages = 5000;
  static constexpr char const* filename = "macros_and_functions_logging_test.log";
  static std::string const logger_name = "logger";

  BackendOptions bo;
  bo.transit_event_buffer_initial_capacity = 2;
  Backend::start(bo);

  Frontend::preallocate();

  // Set writing logging to a file
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');

      // For this test only we use the default buffer size, it should not make any difference it is just for testing the default behaviour and code coverage
      cfg.set_write_buffer_size(0);

      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  logger->set_log_level(LogLevel::Debug);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "This is message {}", i);
    warning(logger, "This is message {}", i);
    LOG_DEBUG(logger, "This is message {}", i);
    error(logger, "This is message {}", i);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages * 4);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string expected_string =
      "LOG_INFO      " + logger_name + "       This is message " + std::to_string(i);
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "LOG_WARNING   " + logger_name + "       This is message " + std::to_string(i);
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "LOG_DEBUG     " + logger_name + "       This is message " + std::to_string(i);
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "LOG_ERROR     " + logger_name + "       This is message " + std::to_string(i);
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  testing::remove_file(filename);
}