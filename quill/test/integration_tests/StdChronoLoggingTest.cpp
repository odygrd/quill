#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Chrono.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_chrono_logging")
{
  static constexpr char const* filename = "std_chrono_logging.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  BackendOptions bo;
  bo.error_notifier = [](std::string const&) {};
  Backend::start(bo);

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

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  std::time_t custom_time = 1659342000;
  std::chrono::system_clock::time_point time_point = std::chrono::system_clock::from_time_t(custom_time);
  LOG_INFO(logger, "time_point [{}]", time_point);

  std::chrono::seconds secs{123};
  LOG_INFO(logger, "seconds [{}]", secs);

  std::chrono::milliseconds ms{1232};
  LOG_INFO(logger, "milliseconds [{}]", ms);

  std::chrono::microseconds us{3213};
  LOG_INFO(logger, "microseconds [{}]", us);

  std::chrono::nanoseconds ns{10};
  LOG_INFO(logger, "nanoseconds [{}]", ns);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();
  REQUIRE_FALSE(Backend::is_running());

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       time_point [";
    expected_string += fmtquill::format("{}", time_point);
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       seconds [";
    expected_string += fmtquill::format("{}", secs);
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       milliseconds [";
    expected_string += fmtquill::format("{}", ms);
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       microseconds [";
    expected_string += fmtquill::format("{}", us);
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       nanoseconds [";
    expected_string += fmtquill::format("{}", ns);
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  testing::remove_file(filename);
}
