#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/JsonConsoleSink.h"

#include "quill/bundled/fmt/format.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("json_console_logging")
{
  static constexpr size_t number_of_messages = 50;
  static std::string const logger_name_a = "logger_a";

  // Start the logging backend thread
  Backend::start();

  quill::testing::CaptureStdout();

  // Set writing logging to a file
  auto console_sink = Frontend::create_or_get_sink<JsonConsoleSink>("json_console");
  Logger* logger_a = Frontend::create_or_get_logger(logger_name_a, console_sink);

  // log a few messages
  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger_a, "Hello log num [{num}, {multiply}, {add}]", i, i * i, i + i);
  }

  // flush all log and remove all loggers
  for (Logger* logger : Frontend::get_all_loggers())
  {
    logger->flush_log();
    Frontend::remove_logger(logger);
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // convert result to vector
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
    std::string const expected_format =
      R"("logger_a","log_level":"INFO","message":"Hello log num [{{num}}, {{multiply}}, {{add}}]","num":"{}","multiply":"{}","add":"{}")";
    std::string const expected_string =
      fmtquill::format(fmtquill::runtime(expected_format), i, i * i, i + i);

    REQUIRE(testing::file_contains(file_contents, expected_string));
  }
}