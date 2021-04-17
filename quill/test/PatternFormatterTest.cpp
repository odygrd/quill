#include "doctest/doctest.h"

#include "quill/PatternFormatter.h"
#include "quill/detail/misc/Macros.h"
#include <chrono>

TEST_SUITE_BEGIN("PatternFormatter");

using namespace quill::detail;
using namespace quill;

char const* thread_name = "test_thread";

TEST_CASE("default_pattern_formatter")
{
  PatternFormatter default_pattern_formatter;

  std::chrono::nanoseconds ts{1579815761000023021};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {} formatter {}", LogLevel::Info};

  // Format to a buffer
  default_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                   "pattern", 1234);

  auto const& formatted_buffer = default_pattern_formatter.formatted_log_record();

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  // Default pattern formatter is using local time to convert the timestamp to timezone, in this test we ignore the timestamp
  std::string const expected_string =
    "[31341] PatternFormatterTest.cpp:21  LOG_INFO      test_logger  - This the pattern formatter "
    "1234\n";
  auto const found_expected = formatted_string.find(expected_string);
  REQUIRE(found_expected != std::string::npos);
}

TEST_CASE("custom_pattern_message_only")
{
  // Message only
  PatternFormatter custom_pattern_formatter{QUILL_STRING("%(message)"), "%H:%M:%S.%Qns", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  // Format to a buffer
  custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                  "pattern", 12.34);

  auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  std::string const expected_string = "This the 12.34 formatter pattern\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_nanoseconds")
{
  // Custom pattern with part 1 and part 3
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING(
      "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) - "
      "%(message) [%(function_name)]"),
    "%m-%d-%Y %H:%M:%S.%Qns", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  // Format to a buffer
  custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                  "pattern", 1234);

  auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:79 LOG_DEBUG     test_logger - "
    "This the 1234 formatter pattern [_DOCTEST_ANON_FUNC_8]\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_microseconds")
{
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING(
      "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) - "
      "%(message) [%(function_name)]"),
    "%m-%d-%Y %H:%M:%S.%Qus", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761020123000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  // Format to a buffer
  custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                  "pattern", 1234);

  auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.020123 [31341] PatternFormatterTest.cpp:110 LOG_DEBUG     test_logger - "
    "This the 1234 formatter pattern [_DOCTEST_ANON_FUNC_10]\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_milliseconds")
{
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING(
      "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) - "
      "%(message) [%(function_name)]"),
    "%m-%d-%Y %H:%M:%S.%Qms", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761099000000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  // Format to a buffer
  custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                  "pattern", 1234);

  auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.099 [31341] PatternFormatterTest.cpp:141 LOG_DEBUG     test_logger - This "
    "the 1234 formatter pattern [_DOCTEST_ANON_FUNC_12]\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_none")
{
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING(
      "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) - "
      "%(message) [%(function_name)]"),
    "%m-%d-%Y %H:%M:%S", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  // Format to a buffer
  custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                  "pattern", 1234);

  auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41 [31341] PatternFormatterTest.cpp:172 LOG_DEBUG     test_logger - This the "
    "1234 formatter pattern [_DOCTEST_ANON_FUNC_14]\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_strftime_reallocation_on_format_string_2")
{
  // set a timestamp_format that will cause timestamp _formatted_date to re-allocate.
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING(
      "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) - "
      "%(message) [%(function_name)]"),
    "%FT%T.%Qus%FT%T", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  for (size_t i = 0; i < 5; ++i)
  {
    // Format to a buffer
    custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                    "pattern", 1234);
    auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

    // Convert the buffer to a string
    std::string const formatted_string = fmt::to_string(formatted_buffer);

    std::string const expected_string =
      "2020-01-23T21:42:41.0992202020-01-23T21:42:41 [31341] PatternFormatterTest.cpp:204 "
      "LOG_DEBUG     test_logger - This the 1234 formatter pattern [_DOCTEST_ANON_FUNC_16]\n";

    REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
    REQUIRE_EQ(formatted_string, expected_string);
  }
}

TEST_CASE("custom_pattern_timestamp_strftime_reallocation_when_adding_fractional_seconds")
{
  // set a timestamp_format that will cause timestamp _formatted_date to re-allocate.
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING(
      "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) - "
      "%(message) [%(function_name)]"),
    "%FT%T.%T.%Qus%FT%T", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  for (size_t i = 0; i < 5; ++i)
  {
    // Format to a buffer
    custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                    "pattern", 1234);
    auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

    // Convert the buffer to a string
    std::string const formatted_string = fmt::to_string(formatted_buffer);

    std::string const expected_string =
      "2020-01-23T21:42:41.21:42:41.0992202020-01-23T21:42:41 [31341] PatternFormatterTest.cpp:238 "
      "LOG_DEBUG     test_logger - This the 1234 formatter pattern [_DOCTEST_ANON_FUNC_18]\n";

    REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
    REQUIRE_EQ(formatted_string, expected_string);
  }
}

#ifndef QUILL_NO_EXCEPTIONS
TEST_CASE("invalid_pattern")
{
  // missing %(message)
  REQUIRE_THROWS_AS(
    PatternFormatter(
      QUILL_STRING("%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                   "[%(function_name)]"),
      "%H:%M:%S.%Qns", Timezone::GmtTime),
    quill::QuillError);

  // missing %)
  REQUIRE_THROWS_AS(
    PatternFormatter(
      QUILL_STRING("%(ascii_time [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                   "%(message) [%(function_name)]"),
      "%H:%M:%S.%Qns", Timezone::GmtTime),
    quill::QuillError);

  // invalid attribute %(invalid)
  REQUIRE_THROWS_AS(
    PatternFormatter(
      QUILL_STRING("%(invalid) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - "
                   "%(message) [%(function_name)]"),
      "%H:%M:%S.%Qns", Timezone::GmtTime),
    quill::QuillError);
}
#endif

TEST_CASE("custom_pattern")
{
  // Custom pattern with part 1 and part 2
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING(
      "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) - "
      "%(message)"),
    "%m-%d-%Y %H:%M:%S.%Qns", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  // Format to a buffer
  custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                  "pattern", 1234);

  auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:301 LOG_DEBUG     test_logger "
    "- This the 1234 formatter pattern\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_part_3_no_format_specifiers")
{
  // Custom pattern with a part 3 that has no format specifiers:
  //   Part 1 - "|{}|{}|"
  //   Part 3 - "|EOM|"
  PatternFormatter custom_pattern_formatter{
    QUILL_STRING("|LOG_%(level_name)|%(logger_name)|%(message)|EOM|"), "%H:%M:%S", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  LogMacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __func__,
                                 "This the {1} formatter {0}", LogLevel::Debug};

  // Format to a buffer
  custom_pattern_formatter.format(ts, thread_id, thread_name, logger_name.data(), log_line_info,
                                  "pattern", 1234);

  auto const& formatted_buffer = custom_pattern_formatter.formatted_log_record();

  // Convert the buffer to a string
  std::string const formatted_string = fmt::to_string(formatted_buffer);

  std::string const expected_string =
    "|LOG_DEBUG    |test_logger|This the 1234 formatter pattern|EOM|\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_SUITE_END();
