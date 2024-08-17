#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("compile_active_log_level")
{
  // we build this target with
  // add_compile_definitions(-DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)
  static constexpr char const* filename = "compile_active_log_level.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();
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

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));
  logger->set_log_level(quill::LogLevel::TraceL3);
  REQUIRE_EQ(logger->get_log_level(), quill::LogLevel::TraceL3);

  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
  LOG_INFO(logger, "This is a log info example {}", 21);
  LOG_WARNING(logger, "This is a log warning example {}", 11);
  LOG_ERROR(logger, "This is a log error example {}", 3212);
  LOG_CRITICAL(logger, "This is a log critical example {}", 321);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check, we only except statements above warning
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 3);

  REQUIRE(quill::testing::file_contains(file_contents, std::string{"This is a log warning example 11"}));

  REQUIRE(quill::testing::file_contains(file_contents, std::string{"This is a log error example 3212"}));

  REQUIRE(quill::testing::file_contains(file_contents, std::string{"This is a log critical example 321"}));

  testing::remove_file(filename);
}