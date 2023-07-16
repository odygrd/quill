#include "doctest/doctest.h"

#include "quill/PatternFormatter.h"
#include "quill/detail/misc/Common.h"
#include <chrono>
#include <string_view>

TEST_SUITE_BEGIN("PatternFormatter");

using namespace quill::detail;
using namespace quill;

char const* thread_name = "test_thread";
std::string_view process_id = "123";

TEST_CASE("default_pattern_formatter")
{
  PatternFormatter default_pattern_formatter;

  std::chrono::nanoseconds ts{1579815761000023021};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {} formatter {}",
                              LogLevel::Info,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  // Format to a buffer
  transit_event_fmt_buffer_t mbuff;
  fmtquill::format_to(std::back_inserter(mbuff), fmtquill::runtime(log_line_info.message_format()),
                      "pattern", 1234);
  auto const& formatted_buffer =
    default_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                     loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  // Default pattern formatter is using local time to convert the timestamp to timezone, in this test we ignore the timestamp
  std::string const expected_string =
    "[31341] PatternFormatterTest.cpp:25  LOG_INFO      test_logger  This the pattern formatter "
    "1234\n";
  auto const found_expected = formatted_string.find(expected_string);
  REQUIRE(found_expected != std::string::npos);
}

TEST_CASE("custom_pattern_message_only")
{
  // Message only
  PatternFormatter custom_pattern_formatter{"%(level_id) %(message)", "%H:%M:%S.%Qns", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  // Format to a buffer
  transit_event_fmt_buffer_t mbuff;
  fmtquill::format_to(std::back_inserter(mbuff), fmtquill::runtime(log_line_info.message_format()),
                      "pattern", 12.34);
  auto const& formatted_buffer =
    custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                    loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string = "D This the 12.34 formatter pattern\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_nanoseconds")
{
  // Custom pattern with part 1 and part 3
  PatternFormatter custom_pattern_formatter{
    "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) "
    "%(message) [%(function_name)]",
    "%m-%d-%Y %H:%M:%S.%Qns", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  // Format to a buffer
  transit_event_fmt_buffer_t mbuff;
  fmtquill::format_to(std::back_inserter(mbuff), fmtquill::runtime(log_line_info.message_format()),
                      "pattern", 1234);
  auto const& formatted_buffer =
    custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                    loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:98 LOG_DEBUG test_logger "
    "This the 1234 formatter pattern [DOCTEST_ANON_FUNC_7]\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_microseconds")
{
  PatternFormatter custom_pattern_formatter{
    "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) "
    "%(message) [%(function_name)]",
    "%m-%d-%Y %H:%M:%S.%Qus", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761020123000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  // Format to a buffer
  transit_event_fmt_buffer_t mbuff;
  fmtquill::format_to(std::back_inserter(mbuff), fmtquill::runtime(log_line_info.message_format()),
                      "pattern", 1234);
  auto const& formatted_buffer =
    custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                    loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.020123 [31341] PatternFormatterTest.cpp:137 LOG_DEBUG test_logger "
    "This the 1234 formatter pattern [DOCTEST_ANON_FUNC_9]\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_milliseconds")
{
  PatternFormatter custom_pattern_formatter{
    "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) "
    "%(message) [%(function_name)]",
    "%m-%d-%Y %H:%M:%S.%Qms", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761099000000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  // Format to a buffer
  transit_event_fmt_buffer_t mbuff;
  fmtquill::format_to(std::back_inserter(mbuff), fmtquill::runtime(log_line_info.message_format()),
                      "pattern", 1234);
  auto const& formatted_buffer =
    custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                    loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.099 [31341] PatternFormatterTest.cpp:176 LOG_DEBUG test_logger This "
    "the 1234 formatter pattern [DOCTEST_ANON_FUNC_11]\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_none")
{
  PatternFormatter custom_pattern_formatter{
    "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) "
    "%(message) [%(function_name)]",
    "%m-%d-%Y %H:%M:%S", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  // Format to a buffer
  transit_event_fmt_buffer_t mbuff;
  fmtquill::format_to(std::back_inserter(mbuff), fmtquill::runtime(log_line_info.message_format()),
                      "pattern", 1234);
  auto const& formatted_buffer =
    custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                    loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41 [31341] PatternFormatterTest.cpp:215 LOG_DEBUG test_logger This the "
    "1234 formatter pattern [DOCTEST_ANON_FUNC_13]\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_strftime_reallocation_on_format_string_2")
{
  // set a timestamp_format that will cause timestamp _formatted_date to re-allocate.
  PatternFormatter custom_pattern_formatter{
    "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) "
    "%(message) [%(function_name)]",
    "%FT%T.%Qus%FT%T", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  for (size_t i = 0; i < 5; ++i)
  {
    // Format to a buffer
    transit_event_fmt_buffer_t mbuff;
    fmtquill::format_to(std::back_inserter(mbuff),
                        fmtquill::runtime(log_line_info.message_format()), "pattern", 1234);
    auto const& formatted_buffer =
      custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                      loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

    // Convert the buffer to a string
    std::string const formatted_string = fmtquill::to_string(formatted_buffer);

    std::string const expected_string =
      "2020-01-23T21:42:41.0992202020-01-23T21:42:41 [31341] PatternFormatterTest.cpp:255 "
      "LOG_DEBUG test_logger This the 1234 formatter pattern [DOCTEST_ANON_FUNC_15]\n";

    REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
    REQUIRE_EQ(formatted_string, expected_string);
  }
}

TEST_CASE("custom_pattern_timestamp_strftime_reallocation_when_adding_fractional_seconds")
{
  // set a timestamp_format that will cause timestamp _formatted_date to re-allocate.
  PatternFormatter custom_pattern_formatter{
    "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) "
    "%(message) [%(function_name)]",
    "%FT%T.%T.%Qus%FT%T", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  for (size_t i = 0; i < 5; ++i)
  {
    // Format to a buffer
    transit_event_fmt_buffer_t mbuff;
    fmtquill::format_to(std::back_inserter(mbuff),
                        fmtquill::runtime(log_line_info.message_format()), "pattern", 1234);
    auto const& formatted_buffer =
      custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                      loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

    // Convert the buffer to a string
    std::string const formatted_string = fmtquill::to_string(formatted_buffer);

    std::string const expected_string =
      "2020-01-23T21:42:41.21:42:41.0992202020-01-23T21:42:41 [31341] PatternFormatterTest.cpp:298 "
      "LOG_DEBUG test_logger This the 1234 formatter pattern [DOCTEST_ANON_FUNC_17]\n";

    REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
    REQUIRE_EQ(formatted_string, expected_string);
  }
}

#ifndef QUILL_NO_EXCEPTIONS
TEST_CASE("invalid_pattern")
{
  // missing %)
  REQUIRE_THROWS_AS(
    PatternFormatter("%(ascii_time [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) "
                     "%(message) [%(function_name)]",
                     "%H:%M:%S.%Qns", Timezone::GmtTime),
    quill::QuillError);

  // invalid attribute %(invalid)
  REQUIRE_THROWS_AS(
    PatternFormatter("%(invalid) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) "
                     "%(message) [%(function_name)]",
                     "%H:%M:%S.%Qns", Timezone::GmtTime),
    quill::QuillError);
}
#endif

TEST_CASE("custom_pattern")
{
  // Custom pattern with part 1 and part 2
  PatternFormatter custom_pattern_formatter{
    "%(ascii_time) [%(thread)] %(filename):%(lineno) LOG_%(level_name) %(logger_name) "
    "%(message)",
    "%m-%d-%Y %H:%M:%S.%Qns", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  // Format to a buffer
  transit_event_fmt_buffer_t mbuff;
  fmtquill::format_to(std::back_inserter(mbuff), fmtquill::runtime(log_line_info.message_format()),
                      "pattern", 1234);
  auto const& formatted_buffer =
    custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                    loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:360 LOG_DEBUG test_logger "
    "This the 1234 formatter pattern\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_part_3_no_format_specifiers")
{
  // Custom pattern with a part 3 that has no format specifiers:
  //   Part 1 - "|{}|{}|"
  //   Part 3 - "|EOM|"
  PatternFormatter custom_pattern_formatter{"|LOG_%(level_name)|%(logger_name)|%(message)|EOM|",
                                            "%H:%M:%S", Timezone::GmtTime};

  std::chrono::nanoseconds ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata log_line_info{QUILL_STRINGIFY(__LINE__),
                              __FILE__,
                              __FILE__ ":" QUILL_STRINGIFY(__LINE__),
                              __func__,
                              "This the {1} formatter {0}",
                              LogLevel::Debug,
                              MacroMetadata::Event::Log,
                              false,
                              false};

  // Format to a buffer
  transit_event_fmt_buffer_t mbuff;
  fmtquill::format_to(std::back_inserter(mbuff), fmtquill::runtime(log_line_info.message_format()),
                      "pattern", 1234);
  auto const& formatted_buffer =
    custom_pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name.data(),
                                    loglevel_to_string(log_line_info.level()), log_line_info, mbuff);

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "|LOG_DEBUG|test_logger|This the 1234 formatter pattern|EOM|\n";

  REQUIRE_EQ(formatted_buffer.size(), expected_string.length());
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_SUITE_END();
