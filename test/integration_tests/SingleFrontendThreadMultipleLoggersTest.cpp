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
TEST_CASE("single_frontend_thread_multiple_loggers")
{
  static constexpr size_t number_of_messages = 500;
  static constexpr char const* filename = "single_frontend_thread_multiple_loggers.log";
  static std::string const logger_name_a = "logger_a";
  static std::string const logger_name_b = "logger_b";

  // Start the logging backend thread
  Backend::start();

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

  Logger* logger_a = Frontend::create_or_get_logger(
    logger_name_a, std::move(file_sink), quill::PatternFormatterOptions("[%(logger)] %(message)"));

  // Take properties from logger_a
  Logger* logger_b = Frontend::create_or_get_logger(logger_name_b, logger_a);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger_a, "This is message {}", i);
    LOG_INFO(logger_b, "This is message {}", i);
  }

  logger_a->flush_log();
  Frontend::remove_logger(logger_a);
  Frontend::remove_logger(logger_b);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages * 2);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    {
      std::string expected_string =
        std::string("[") + logger_name_a + "] This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }

    {
      std::string expected_string =
        std::string("[") + logger_name_b + "] This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  testing::remove_file(filename);
}