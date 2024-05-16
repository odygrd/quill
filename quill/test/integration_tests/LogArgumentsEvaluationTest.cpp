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
TEST_CASE("log_arguments_evaluation")
{
  static constexpr char const* filename = "log_arguments_evaluation.log";

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

  std::string const logger_name = "logger";
  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  size_t cnt{0};
  auto arg_str = [&cnt]()
  {
    ++cnt;
    return "expensive_calculation";
  };

  // 1. checks that log arguments are not evaluated when we don't log
  logger->set_log_level(quill::LogLevel::Info);
  LOG_DEBUG(logger, "Test log arguments {}", arg_str());
  LOG_TRACE_L1(logger, "Test log arguments {}", arg_str());
  REQUIRE_EQ(cnt, 0);

  // 2. checks that log arguments are evaluated only once per log statement
  LOG_INFO(logger, "Test log arguments {}", arg_str());
  REQUIRE_EQ(cnt, 1);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 1);

  testing::remove_file(filename);
}