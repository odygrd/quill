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
TEST_CASE("remove_logger_blocking")
{
  static constexpr size_t number_of_messages = 10;
  static constexpr char const* filename = "remove_logger_blocking.log";
  static std::string const logger_name = "logger";

  for (size_t iter = 0; iter < 5; ++iter)
  {
    // Start the logging backend thread
    Backend::start();

    // Set writing logging to a file
    auto file_sink = Frontend::create_or_get_sink<FileSink>(
      filename,
      []()
      {
        FileSinkConfig cfg;
        cfg.set_open_mode('w');

        // For this test only we use the default buffer size, it should not make any difference it
        // is just for testing the default behaviour and code coverage
        cfg.set_write_buffer_size(0);

        return cfg;
      }(),
      FileEventNotifier{});

    // We create a new logger each iteration here with the iter in the format pattern for testing
    std::string const iter_str = std::to_string(iter) + "_ITER";

    Logger* logger = Frontend::create_or_get_logger(
      logger_name, std::move(file_sink), quill::PatternFormatterOptions{iter_str + " %(message)"});

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      LOG_INFO(logger, "This is message {}", i);
    }

    // Remove logger should also close the Sink and the file
    Frontend::remove_logger_blocking(logger);

    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), number_of_messages);

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      std::string expected_string = iter_str + " This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  // Wait until backend stops
  Backend::stop();

  // Remove the file
  testing::remove_file(filename);
}