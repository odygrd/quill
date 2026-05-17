#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Chrono.h"

#include <cstdio>
#include <ctime>
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

  // Test rvalue with chrono duration
  auto rvalue_duration = std::chrono::seconds(100);
  LOG_INFO(logger, "rvalue_duration [{}]", std::move(rvalue_duration));

  std::tm tm_value{};
  tm_value.tm_sec = 45;
  tm_value.tm_min = 30;
  tm_value.tm_hour = 12;
  tm_value.tm_mday = 9;
  tm_value.tm_mon = 7;    // August (0-based)
  tm_value.tm_year = 124; // 2024
  tm_value.tm_wday = 5;   // Friday
  tm_value.tm_yday = 221;
  tm_value.tm_isdst = 0;
  LOG_INFO(logger, "tm [{:%F %T}]", tm_value);

#if QUILL_HAS_CXX20_CHRONO
  std::chrono::year const year{2024};
  LOG_INFO(logger, "year [{}]", year);

  std::chrono::month const month{8};
  LOG_INFO(logger, "month [{}]", month);

  std::chrono::day const day{9};
  LOG_INFO(logger, "day [{}]", day);

  std::chrono::weekday const weekday{5};
  LOG_INFO(logger, "weekday [{}]", weekday);

  std::chrono::year_month_day const year_month_day{year, month, day};
  LOG_INFO(logger, "year_month_day [{}]", year_month_day);
#endif

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

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       rvalue_duration [";
    expected_string += fmtquill::format("{}", std::chrono::seconds(100));
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       tm [";
    expected_string += fmtquill::format("{:%F %T}", tm_value);
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

#if QUILL_HAS_CXX20_CHRONO
  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       year [";
    expected_string += fmtquill::format("{}", std::chrono::year{2024});
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       month [";
    expected_string += fmtquill::format("{}", std::chrono::month{8});
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       day [";
    expected_string += fmtquill::format("{}", std::chrono::day{9});
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       weekday [";
    expected_string += fmtquill::format("{}", std::chrono::weekday{5});
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string expected_string = "LOG_INFO      " + logger_name + "       year_month_day [";
    expected_string += fmtquill::format(
      "{}", std::chrono::year_month_day{std::chrono::year{2024}, std::chrono::month{8}, std::chrono::day{9}});
    expected_string += "]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }
#endif

  testing::remove_file(filename);
}
