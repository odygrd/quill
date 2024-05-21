#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("multiple_sinks_same_logger")
{
  static constexpr size_t number_of_messages = 10000;
  static constexpr char const* filename = "multiple_sinks_same_logger.log";

  // Start the logging backend thread
  Backend::start();

  quill::testing::CaptureStdout();

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

  bool const using_colours = false;
  std::string const stream = "stdout";
  auto console_sink = Frontend::create_or_get_sink<ConsoleSink>("console_sink", using_colours, stream);

  std::string const logger_name = "logger";
  Logger* logger =
    Frontend::create_or_get_logger(logger_name, {std::move(file_sink), std::move(console_sink)});

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "This is message {}", i);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

    REQUIRE_EQ(file_contents.size(), number_of_messages);

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      std::string expected_string = logger_name + "       This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  {
    // convert stdout result to vector
    std::string results = quill::testing::GetCapturedStdout();
    std::stringstream data(results);

    std::string line;
    std::vector<std::string> file_contents;
    while (std::getline(data, line, '\n'))
    {
      file_contents.push_back(line);
    }

    REQUIRE_EQ(file_contents.size(), number_of_messages);

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      std::string expected_string = "logger       This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  testing::remove_file(filename);
}