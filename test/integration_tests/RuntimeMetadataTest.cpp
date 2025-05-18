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

void log_runtime_message(quill::Logger* quill_logger, quill::LogLevel log_level,
                         char const* filename, uint32_t lineno, std::string_view arg)
{
  QUILL_LOG_RUNTIME_METADATA(quill_logger, log_level, filename, lineno, "", "{}", arg);
}

/***/
TEST_CASE("runtime_metadata")
{
  static constexpr char const* filename = "runtime_metadata.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  // Set writing logging to a file
  auto file_sink = Frontend::create_or_get_sink<FileSink>(filename,
                                                          []()
                                                          {
                                                            FileSinkConfig cfg;
                                                            cfg.set_open_mode('w');
                                                            return cfg;
                                                          }());

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::move(file_sink),
    PatternFormatterOptions{
      "%(short_source_location) %(caller_function) LOG_%(log_level:<9) %(logger:<12) "
      "%(message)"});

  LOG_RUNTIME_METADATA(logger, quill::LogLevel::Info, "RuntimeMetadataTest.cpp", 1234, "function_1",
                       "{}", "test message");

  LOG_RUNTIME_METADATA(logger, quill::LogLevel::Warning, "RuntimeMetadataTest.cpp", 1234,
                       "function_1", "{}", "test message");

  LOG_RUNTIME_METADATA(logger, quill::LogLevel::Info, "RuntimeMetadataTest.cpp", 1234, "foo()",
                       "{}", "test message");

  LOG_INFO(logger, "standard message {} {}", 123, 456);

  LOG_RUNTIME_METADATA(logger, quill::LogLevel::Info, "app.cpp", 98, "foo()",
                       "Runtime metadata with {} {}", 2, 3);

  std::string file_name = "RuntimeMetadataTest.cpp";
  uint32_t line_number = 98;
  std::string function_name = "function_1";
  uint32_t a = 1;
  uint32_t b = 2;
  LOG_RUNTIME_METADATA(logger, quill::LogLevel::Info, file_name.data(), line_number,
                       function_name.data(), "a={} and b={}", a, b);

  log_runtime_message(logger, quill::LogLevel::Info, "app.cpp", 1234, "test message");

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 7);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"RuntimeMetadataTest.cpp:1234 function_1 LOG_INFO      logger       test message"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"RuntimeMetadataTest.cpp:1234 function_1 LOG_WARNING   logger       test message"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"RuntimeMetadataTest.cpp:1234 foo() LOG_INFO      logger       test message"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      logger       standard message 123 456"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"app.cpp:98 foo() LOG_INFO      logger       Runtime metadata with 2 3"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"app.cpp:1234  LOG_INFO      logger       test message"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"RuntimeMetadataTest.cpp:98 function_1 LOG_INFO      logger       a=1 and b=2"}));

  testing::remove_file(filename);
}