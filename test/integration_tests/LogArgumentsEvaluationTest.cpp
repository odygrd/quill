#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <chrono>
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

  // 3. checks that public logger and runtime-level expressions are evaluated only once
  size_t logger_evaluation_count{0};
  auto logger_expression = [&]() -> Logger*
  {
    ++logger_evaluation_count;
    return logger;
  };

  LOG_INFO(logger_expression(), "Normal logger expression");
  REQUIRE_EQ(logger_evaluation_count, 1);

  LOG_INFO_LIMIT(std::chrono::nanoseconds{0}, logger_expression(),
                 "Rate limited logger expression {}", 1);
  REQUIRE_EQ(logger_evaluation_count, 2);

  LOG_INFO_LIMIT_EVERY_N(1, logger_expression(), "Every N logger expression {}", 1);
  REQUIRE_EQ(logger_evaluation_count, 3);

  logger->init_backtrace(1, LogLevel::Error);
  LOG_BACKTRACE(logger_expression(), "Backtrace logger expression");
  REQUIRE_EQ(logger_evaluation_count, 4);

  size_t log_level_evaluation_count{0};
  auto log_level_expression = [&]()
  {
    ++log_level_evaluation_count;
    return LogLevel::Info;
  };

  LOG_DYNAMIC(logger_expression(), log_level_expression(), "Runtime logger and level expressions");
  REQUIRE_EQ(logger_evaluation_count, 5);
  REQUIRE_EQ(log_level_evaluation_count, 1);

  LOG_ERROR(logger, "Flush backtrace");

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 7);

  testing::remove_file(filename);
}
