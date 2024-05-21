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
TEST_CASE("sink_log_level_filter")
{
  static constexpr char const* filename_a = "sink_log_level_filter_a.log";
  static constexpr char const* filename_b = "sink_log_level_filter_b.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  // Set writing logging to a file
  auto file_sink_a = Frontend::create_or_get_sink<FileSink>(
    filename_a,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  // Set filters to our handlers
  REQUIRE_EQ(file_sink_a->get_log_level_filter(), LogLevel::TraceL3);
  file_sink_a->set_log_level_filter(LogLevel::Info);
  REQUIRE_EQ(file_sink_a->get_log_level_filter(), LogLevel::Info);

  auto file_sink_b = Frontend::create_or_get_sink<FileSink>(
    filename_b,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  // log to filename_b warning, error, critical
  REQUIRE_EQ(file_sink_b->get_log_level_filter(), LogLevel::TraceL3);
  file_sink_b->set_log_level_filter(LogLevel::Error);
  REQUIRE_EQ(file_sink_b->get_log_level_filter(), LogLevel::Error);

  Logger* logger =
    Frontend::create_or_get_logger(logger_name, {std::move(file_sink_a), std::move(file_sink_b)});

  LOG_INFO(logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
  LOG_ERROR(logger, "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

  // Let all log get flushed to the file
  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents_a = quill::testing::file_contents(filename_a);
  REQUIRE_EQ(file_contents_a.size(), 2);
  REQUIRE(quill::testing::file_contains(
    file_contents_a,
    std::string{"LOG_INFO      " + logger_name + "       Lorem ipsum dolor sit amet, consectetur adipiscing elit"}));
  REQUIRE(quill::testing::file_contains(
    file_contents_a,
    std::string{"LOG_ERROR     " + logger_name +
                "       Nulla tempus, libero at dignissim viverra, lectus libero finibus ante"}));

  std::vector<std::string> const file_contents_b = quill::testing::file_contents(filename_b);
  REQUIRE_EQ(file_contents_b.size(), 1);
  REQUIRE(quill::testing::file_contains(
    file_contents_b,
    std::string{"LOG_ERROR     " + logger_name +
                "       Nulla tempus, libero at dignissim viverra, lectus libero finibus ante"}));

  testing::remove_file(filename_a);
  testing::remove_file(filename_b);
}