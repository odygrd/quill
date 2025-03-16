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
TEST_CASE("override_sink_formatter")
{
  static constexpr size_t number_of_messages = 10;
  static constexpr char const* filename_1 = "override_sink_formatter_1.log";
  static constexpr char const* filename_2 = "override_sink_formatter_2.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  // Set writing logging to a file
  auto file_sink_1 = Frontend::create_or_get_sink<FileSink>(
    filename_1,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');

      // For this test only we use the default buffer size, it should not make any difference it is just for testing the default behaviour and code coverage
      cfg.set_write_buffer_size(0);

      return cfg;
    }(),
    FileEventNotifier{});

  auto file_sink_2 = Frontend::create_or_get_sink<FileSink>(
    filename_2,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');

      // For this test only we use the default buffer size, it should not make any difference it is just for testing the default behaviour and code coverage
      cfg.set_write_buffer_size(0);
      cfg.set_override_pattern_formatter_options(
        quill::PatternFormatterOptions{"[OVERRIDE] %(message)"});
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger =
    Frontend::create_or_get_logger(logger_name, {std::move(file_sink_1), std::move(file_sink_2)},
                                   quill::PatternFormatterOptions{"[LOGGER] %(message)"});

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "This is message {}", i);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  {
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_1);
    REQUIRE_EQ(file_contents.size(), number_of_messages);

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      std::string expected_string = "[LOGGER] This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  {
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_2);
    REQUIRE_EQ(file_contents.size(), number_of_messages);

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      std::string expected_string = "[OVERRIDE] This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  testing::remove_file(filename_1);
  testing::remove_file(filename_2);
}