#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/JsonSink.h"

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
  BackendOptions bo;

  // Silence the error_notifier as we are capturing the stdout in the test, we do not want extra messages
  bo.error_notifier = [](std::string const&) {};

  Backend::start(bo);

  quill::testing::CaptureStdout();

  // Set writing logging to a file
  auto console_sink = Frontend::create_or_get_sink<JsonConsoleSink>("json_console");
  Logger* logger_a = Frontend::create_or_get_logger(logger_name_a, console_sink);

  // log a few messages
  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger_a, "Hello log num [{num}, {multiply}, {add}]", i, i * i, i + i);
  }

  LOG_INFO(logger_a, "Reserved {timestamp} {timestamp_1} {logger}", "user timestamp",
           "user timestamp 1", "user logger");

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

  REQUIRE_EQ(file_contents.size(), number_of_messages + 1);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string const expected_format =
      R"("logger_a","log_level":"INFO","message":"Hello log num [{{num}}, {{multiply}}, {{add}}]","num":"{}","multiply":"{}","add":"{}")";
    std::string const expected_string =
      fmtquill::format(fmtquill::runtime(expected_format), i, i * i, i + i);

    REQUIRE(testing::file_contains(file_contents, expected_string));
  }

  REQUIRE(testing::file_contains(
    file_contents, R"("message":"Reserved {timestamp} {timestamp_1} {logger}","timestamp_2":"user timestamp","timestamp_1":"user timestamp 1","logger_1":"user logger")"));
}
