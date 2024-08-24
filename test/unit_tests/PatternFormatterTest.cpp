#include "doctest/doctest.h"

#include "quill/backend/PatternFormatter.h"
#include "quill/core/Common.h"
#include "quill/core/MacroMetadata.h"
#include <chrono>
#include <string_view>
#include <vector>

TEST_SUITE_BEGIN("PatternFormatter");

using namespace quill::detail;
using namespace quill;

char const* thread_name = "test_thread";
std::string_view process_id = "123";

TEST_CASE("default_pattern_formatter")
{
  PatternFormatter default_pattern_formatter{PatternFormatterOptions{}};

  uint64_t const ts{1579815761000023021};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {} formatter {}",
                               nullptr,
                               LogLevel::Info,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = default_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  // Default pattern formatter is using local time to convert the timestamp to timezone, in this test we ignore the timestamp
  std::string const expected_string =
    "[31341] PatternFormatterTest.cpp:25  LOG_INFO      test_logger  This the pattern formatter "
    "1234\n";
  auto const found_expected = formatted_string.find(expected_string);
  REQUIRE_NE(found_expected, std::string::npos);
}

TEST_CASE("custom_pattern_message_only")
{
  // Message only
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "%(log_level_short_code) %(message)", "%H:%M:%S.%Qns", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 12.34);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);
  std::string const expected_string = "D This the 12.34 formatter pattern\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_nanoseconds")
{
  // Custom pattern with part 1 and part 3
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "%(time) [%(thread_id)] %(file_name):%(line_number) LOG_%(log_level) %(logger) "
    "%(message) [%(caller_function)]",
    "%m-%d-%Y %H:%M:%S.%Qns", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:99 LOG_DEBUG test_logger "
    "This the 1234 formatter pattern [DOCTEST_ANON_FUNC_7]\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_microseconds")
{
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "%(time) [%(thread_id)] %(file_name):%(line_number) LOG_%(log_level) %(logger) "
    "%(message) [%(caller_function)]",
    "%m-%d-%Y %H:%M:%S.%Qus", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761020123000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.020123 [31341] PatternFormatterTest.cpp:137 LOG_DEBUG test_logger "
    "This the 1234 formatter pattern [DOCTEST_ANON_FUNC_9]\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_milliseconds")
{
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "%(time) [%(thread_id)] %(file_name):%(line_number) LOG_%(log_level) %(logger) "
    "%(message) [%(caller_function)]",
    "%m-%d-%Y %H:%M:%S.%Qms", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761099000000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.099 [31341] PatternFormatterTest.cpp:175 LOG_DEBUG test_logger This "
    "the 1234 formatter pattern [DOCTEST_ANON_FUNC_11]\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_precision_none")
{
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "%(time) [%(thread_id)] %(file_name):%(line_number) LOG_%(log_level) %(logger) "
    "%(message) [%(caller_function)]",
    "%m-%d-%Y %H:%M:%S", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41 [31341] PatternFormatterTest.cpp:213 LOG_DEBUG test_logger This the "
    "1234 formatter pattern [DOCTEST_ANON_FUNC_13]\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_timestamp_strftime_reallocation_on_format_string_2")
{
  // set a timestamp_format that will cause timestamp _formatted_date to re-allocate.
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "%(time) [%(thread_id)] %(file_name):%(line_number) LOG_%(log_level) %(logger) "
    "%(message) [%(caller_function)]",
    "%FT%T.%Qus%FT%T", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  for (size_t i = 0; i < 5; ++i)
  {
    // Format to a buffer
    fmtquill::memory_buffer log_msg;
    fmtquill::format_to(std::back_inserter(log_msg),
                        fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

    std::vector<std::pair<std::string, std::string>> named_args;

    auto const& formatted_buffer = custom_pattern_formatter.format(
      ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata,
      &named_args, std::string_view{log_msg.data(), log_msg.size()});

    // Convert the buffer to a string
    std::string const formatted_string = fmtquill::to_string(formatted_buffer);

    std::string const expected_string =
      "2020-01-23T21:42:41.0992202020-01-23T21:42:41 [31341] PatternFormatterTest.cpp:252 "
      "LOG_DEBUG test_logger This the 1234 formatter pattern [DOCTEST_ANON_FUNC_15]\n";

    REQUIRE_EQ(formatted_string, expected_string);
  }
}

TEST_CASE("custom_pattern_timestamp_strftime_reallocation_when_adding_fractional_seconds")
{
  // set a timestamp_format that will cause timestamp _formatted_date to re-allocate.
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "%(time) [%(thread_id)] %(file_name):%(line_number) LOG_%(log_level) %(logger) "
    "%(message) [%(caller_function)]",
    "%FT%T.%T.%Qus%FT%T", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761099220000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  for (size_t i = 0; i < 5; ++i)
  {
    // Format to a buffer
    fmtquill::memory_buffer log_msg;
    fmtquill::format_to(std::back_inserter(log_msg),
                        fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

    std::vector<std::pair<std::string, std::string>> named_args;

    auto const& formatted_buffer = custom_pattern_formatter.format(
      ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata,
      &named_args, std::string_view{log_msg.data(), log_msg.size()});

    // Convert the buffer to a string
    std::string const formatted_string = fmtquill::to_string(formatted_buffer);

    std::string const expected_string =
      "2020-01-23T21:42:41.21:42:41.0992202020-01-23T21:42:41 [31341] PatternFormatterTest.cpp:294 "
      "LOG_DEBUG test_logger This the 1234 formatter pattern [DOCTEST_ANON_FUNC_17]\n";

    REQUIRE_EQ(formatted_string, expected_string);
  }
}

#ifndef QUILL_NO_EXCEPTIONS
TEST_CASE("invalid_pattern")
{
  // missing %)
  REQUIRE_THROWS_AS(PatternFormatter(PatternFormatterOptions{
                      "%(time [%(thread_id)] %(file_name):%(line_number) %(log_level) %(logger) "
                      "%(message) [%(caller_function)]",
                      "%H:%M:%S.%Qns", Timezone::GmtTime, false}),
                    quill::QuillError);

  // invalid attribute %(invalid)
  REQUIRE_THROWS_AS(
    PatternFormatter(PatternFormatterOptions{
      "%(invalid) [%(thread_id)] %(file_name):%(line_number) %(log_level) %(logger) "
      "%(message) [%(caller_function)]",
      "%H:%M:%S.%Qns", Timezone::GmtTime, false}),
    quill::QuillError);
}
#endif

TEST_CASE("custom_pattern")
{
  // Custom pattern with part 1 and part 2
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "%(time) [%(thread_id)] %(file_name):%(line_number) LOG_%(log_level) %(logger) %(message)",
    "%m-%d-%Y %H:%M:%S.%Qns", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:355 LOG_DEBUG test_logger "
    "This the 1234 formatter pattern\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("custom_pattern_part_3_no_format_specifiers")
{
  // Custom pattern with a part 3 that has no format specifiers:
  //   Part 1 - "|{}|{}|"
  //   Part 3 - "|EOM|"
  PatternFormatter custom_pattern_formatter{PatternFormatterOptions{
    "|LOG_%(log_level)|%(logger)|%(message)|EOM|", "%H:%M:%S", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "|LOG_DEBUG|test_logger|This the 1234 formatter pattern|EOM|\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("empty_format_pattern")
{
  PatternFormatter empty_formatter{PatternFormatterOptions{"", "%H:%M:%S", Timezone::GmtTime, false}};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer =
    empty_formatter.format(ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata,
                           &named_args, std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string = {};

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("pattern_timestamp_move_constructor")
{
  // Custom pattern with part 1 and part 3
  PatternFormatter pattern_formatter{PatternFormatterOptions{
    "%(time) [%(thread_id)] %(file_name):%(line_number) LOG_%(log_level) %(logger) "
    "%(message) [%(caller_function)]",
    "%m-%d-%Y %H:%M:%S.%Qns", Timezone::GmtTime, false}};

  PatternFormatter pattern_formatter_move{std::move(pattern_formatter)};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "This the {1} formatter {0}",
                               nullptr,
                               LogLevel::Debug,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "pattern", 1234);

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = pattern_formatter_move.format(
    ts, thread_id, thread_name, process_id, logger_name, "DEBUG", "D", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  std::string const expected_string =
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:467 LOG_DEBUG test_logger "
    "This the 1234 formatter pattern [DOCTEST_ANON_FUNC_27]\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_SUITE_END();
