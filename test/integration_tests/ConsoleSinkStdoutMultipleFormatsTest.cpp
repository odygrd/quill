#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/ranges.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("console_sink_stdout_multiple_formats")
{
  static std::string const logger_name_a = "logger_a";
  static std::string const logger_name_b = "logger_b";

  // Start the logging backend thread
  Backend::start();

  quill::testing::CaptureStdout();

  // Set writing logging to a file
  bool const using_colours = false;
  std::string const stream = "stdout";
  auto console_sink = Frontend::create_or_get_sink<ConsoleSink>("console_sink", using_colours, stream);

  Logger* logger_a = Frontend::create_or_get_logger(logger_name_a, console_sink);
  Logger* logger_b = Frontend::create_or_get_logger(logger_name_b, console_sink, quill::PatternFormatterOptions{"%(logger) - %(message) (%(caller_function))"});

  console_sink.reset();

  // log a few messages so we rotate files
  for (size_t i = 0; i < 20; ++i)
  {
    if (i % 2 == 0)
    {
      LOG_INFO(logger_a, "Hello log num {}", i);
    }
    else
    {
      LOG_INFO(logger_b, "Hello log num {}", i);
    }
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
  std::vector<std::string> result_arr;
  while (std::getline(data, line, '\n'))
  {
    result_arr.push_back(line);
  }

  for (uint32_t i = 0; i < 20; ++i)
  {
    if (i % 2 == 0)
    {
      std::string expected_string =
        "LOG_INFO      " + logger_name_a + "     Hello log num " + std::to_string(i);

      if (!quill::testing::file_contains(result_arr, expected_string))
      {
        FAIL(fmtquill::format("expected [{}] is not in results [{}]", expected_string, result_arr).data());
      }
    }
    else
    {
      std::string expected_string =
        logger_name_b + " - Hello log num " + std::to_string(i) + " (DOCTEST_ANON_FUNC_2)";

      if (!quill::testing::file_contains(result_arr, expected_string))
      {
        FAIL(fmtquill::format("expected [{}] is not in results [{}]", expected_string, result_arr).data());
      }
    }
  }
}