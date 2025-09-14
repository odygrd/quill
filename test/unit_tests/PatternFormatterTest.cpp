#include "doctest/doctest.h"

#include "quill/backend/PatternFormatter.h"
#include "quill/core/Common.h"
#include "quill/core/Filesystem.h"
#include "quill/core/MacroMetadata.h"
#include <chrono>
#include <string>
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
    "[31341] PatternFormatterTest.cpp:27  LOG_INFO      test_logger  This the pattern formatter "
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
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:101 LOG_DEBUG test_logger "
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
    "01-23-2020 21:42:41.020123 [31341] PatternFormatterTest.cpp:139 LOG_DEBUG test_logger "
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
    "01-23-2020 21:42:41.099 [31341] PatternFormatterTest.cpp:177 LOG_DEBUG test_logger This "
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
    "01-23-2020 21:42:41 [31341] PatternFormatterTest.cpp:215 LOG_DEBUG test_logger This the "
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
      "2020-01-23T21:42:41.0992202020-01-23T21:42:41 [31341] PatternFormatterTest.cpp:254 "
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
      "2020-01-23T21:42:41.21:42:41.0992202020-01-23T21:42:41 [31341] PatternFormatterTest.cpp:296 "
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
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:357 LOG_DEBUG test_logger "
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
    "01-23-2020 21:42:41.000023000 [31341] PatternFormatterTest.cpp:469 LOG_DEBUG test_logger "
    "This the 1234 formatter pattern [DOCTEST_ANON_FUNC_27]\n";

  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("pattern_formatter_source_location_prefix")
{
  std::vector<std::pair<std::string, std::string>> named_args;
  uint64_t const ts{1579815761000023021};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{
    __FILE__ ":" QUILL_STRINGIFY(__LINE__), __func__, "{}}", nullptr, LogLevel::Info, MacroMetadata::Event::Log};

  PatternFormatterOptions po;
  po.format_pattern = "%(source_location)";

  // Find the parent directory that contains the "test" directory
  fs::path repo_root = __FILE__;
  while (repo_root.has_parent_path() && !fs::exists(repo_root / "test"))
  {
    // Extract the repository name (e.g., "quill", "quill-10.0.0") dynamically from the path
    // This ensures the correct prefix stripping regardless of the repository's actual name
    repo_root = repo_root.parent_path();
  }

  {
    po.source_location_path_strip_prefix = repo_root.filename().string();
    PatternFormatter pattern_formatter{po};

    auto const& formatted_buffer =
      pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name, "INFO", "I",
                               macro_metadata, &named_args, std::string_view{});

    std::string const formatted_string = fmtquill::to_string(formatted_buffer);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected_string = "test\\unit_tests\\PatternFormatterTest.cpp:504\n";
#else
    std::string const expected_string = "test/unit_tests/PatternFormatterTest.cpp:504\n";
#endif

    REQUIRE_EQ(formatted_string, expected_string);
  }

  {
    po.source_location_path_strip_prefix =
      repo_root.filename().string() + static_cast<char>(detail::PATH_PREFERRED_SEPARATOR);
    PatternFormatter pattern_formatter{po};

    auto const& formatted_buffer =
      pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name, "INFO", "I",
                               macro_metadata, &named_args, std::string_view{});

    std::string const formatted_string = fmtquill::to_string(formatted_buffer);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected_string = "test\\unit_tests\\PatternFormatterTest.cpp:504\n";
#else
    std::string const expected_string = "test/unit_tests/PatternFormatterTest.cpp:504\n";
#endif

    REQUIRE_EQ(formatted_string, expected_string);
  }
}

TEST_CASE("pattern_formatter_process_function_name")
{
  std::vector<std::pair<std::string, std::string>> named_args;
  uint64_t const ts{1579815761000023021};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{
    __FILE__ ":" QUILL_STRINGIFY(__LINE__), __func__, "{}}", nullptr, LogLevel::Info, MacroMetadata::Event::Log};

  PatternFormatterOptions po;
  po.format_pattern = "%(caller_function)";
  po.process_function_name = [](char const* function_name)
  {
    std::string_view sv{function_name};
    // Check if the string ends with a number after an underscore
    size_t pos = sv.rfind('_');
    if (pos != std::string_view::npos && pos < sv.length() - 1)
    {
      // Check if the characters after the underscore are all digits
      bool all_digits = true;
      for (size_t i = pos + 1; i < sv.length(); ++i)
      {
        if (!std::isdigit(sv[i]))
        {
          all_digits = false;
          break;
        }
      }

      // If they are all digits, remove the underscore and the digits
      if (all_digits)
      {
        return sv.substr(0, pos);
      }
    }
    return sv;
  };

  PatternFormatter pattern_formatter{po};

  auto const& formatted_buffer =
    pattern_formatter.format(ts, thread_id, thread_name, process_id, logger_name, "INFO", "I",
                             macro_metadata, &named_args, std::string_view{});

  // in this test we remove e.g. _31 from the function name with process_function_name
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);
  std::string const expected_string = "DOCTEST_ANON_FUNC\n";
  REQUIRE_EQ(formatted_string, expected_string);
}

struct PatternFormatterMock : public PatternFormatter
{
  PatternFormatterMock() : PatternFormatter(PatternFormatterOptions{}) {};

  std::string process_source_location_path(std::string_view source_location,
                                           std::string const& strip_prefix, bool remove_relative_paths) const
  {
    // windows complains for REQUIRE_EQ(std::string, std::string_view)
    return std::string{PatternFormatter::_process_source_location_path(
      source_location, strip_prefix, remove_relative_paths)};
  }
};

TEST_CASE("process_source_location_path")
{
  PatternFormatterMock formatter;

  // Test with basic prefix
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\home\\user\\quill\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/home/user/quill/test/unit_tests/file.cpp";
#endif

    std::string prefix = "quill";
    bool remove_relative = false;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected = "test\\unit_tests\\file.cpp";
#else
    std::string const expected = "test/unit_tests/file.cpp";
#endif

    REQUIRE_EQ(result, expected);
  }

  // Test with relative paths before prefix
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\..\\..\\quill\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/../../quill/test/unit_tests/file.cpp";
#endif

    std::string prefix = "quill";
    bool remove_relative = true;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected = "test\\unit_tests\\file.cpp";
#else
    std::string const expected = "test/unit_tests/file.cpp";
#endif

    REQUIRE_EQ(result, expected);
  }

  // Test with relative paths before prefix, but without removing relative paths
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\..\\..\\quill\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/../../quill/test/unit_tests/file.cpp";
#endif

    std::string prefix = "quill";
    bool remove_relative = false;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected = "test\\unit_tests\\file.cpp";
#else
    std::string const expected = "test/unit_tests/file.cpp";
#endif

    REQUIRE_EQ(result, expected);
  }

  // Test with prefix including separator
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\home\\user\\quill\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/home/user/quill/test/unit_tests/file.cpp";
#endif

    std::string prefix = std::string{"quill"} + static_cast<char>(detail::PATH_PREFERRED_SEPARATOR);
    bool remove_relative = false;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected = "test\\unit_tests\\file.cpp";
#else
    std::string const expected = "test/unit_tests/file.cpp";
#endif

    REQUIRE_EQ(result, expected);
  }

  // Test with prefix not found
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\home\\user\\quill\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/home/user/quill/test/unit_tests/file.cpp";
#endif

    std::string prefix = "notfound";
    bool remove_relative = false;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);
    REQUIRE_EQ(result, path);
  }

  // Test with empty prefix
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\home\\user\\quill\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/home/user/quill/test/unit_tests/file.cpp";
#endif

    std::string prefix{};
    bool remove_relative = false;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);
    REQUIRE_EQ(result, path);
  }

  // Test with relative path
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "..\\..\\..\\test\\unit_tests\\file.cpp";
#else
    char const* path = "../../../test/unit_tests/file.cpp";
#endif

    std::string prefix{};
    bool remove_relative = true;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected = "test\\unit_tests\\file.cpp";
#else
    std::string const expected = "test/unit_tests/file.cpp";
#endif

    REQUIRE_EQ(result, expected);
  }

  // Test with relative path in the middle
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\home\\user\\..\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/home/user/../test/unit_tests/file.cpp";
#endif

    std::string prefix{};
    bool remove_relative = true;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected = "test\\unit_tests\\file.cpp";
#else
    std::string const expected = "test/unit_tests/file.cpp";
#endif

    REQUIRE_EQ(result, expected);
  }

  // Test with no relative path
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\home\\user\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/home/user/test/unit_tests/file.cpp";
#endif

    std::string prefix{};
    bool remove_relative = true;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);
    REQUIRE_EQ(result, path);
  }

  // Test with both prefix and relative path
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\home\\user\\quill\\..\\..\\..\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/home/user/quill/../../../test/unit_tests/file.cpp";
#endif

    std::string prefix = "quill";
    bool remove_relative = true;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected = "test\\unit_tests\\file.cpp";
#else
    std::string const expected = "test/unit_tests/file.cpp";
#endif

    REQUIRE_EQ(result, expected);
  }

  // Test with both prefix and relative path false
  {
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    char const* path = "\\home\\user\\quill\\..\\..\\..\\test\\unit_tests\\file.cpp";
#else
    char const* path = "/home/user/quill/../../../test/unit_tests/file.cpp";
#endif

    std::string prefix = "quill";
    bool remove_relative = false;

    auto result = formatter.process_source_location_path(path, prefix, remove_relative);

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
    std::string const expected = "..\\..\\..\\test\\unit_tests\\file.cpp";
#else
    std::string const expected = "../../../test/unit_tests/file.cpp";
#endif

    REQUIRE_EQ(result, expected);
  }
}

TEST_CASE("pattern_suffix_nullopt")
{
  PatternFormatterOptions options{"%(message)", "%H:%M:%S", Timezone::GmtTime, false,
                                  PatternFormatterOptions::NO_SUFFIX};
  PatternFormatter custom_pattern_formatter{options};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "Test message without newline",
                               nullptr,
                               LogLevel::Info,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg), fmtquill::runtime(macro_metadata.message_format()));

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  // Should have no pattern suffix
  std::string const expected_string = "Test message without newline";
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("pattern_suffix_custom_char")
{
  // Test with custom pattern suffix
  PatternFormatterOptions options{"%(message)", "%H:%M:%S", Timezone::GmtTime, false, '|'};
  PatternFormatter custom_pattern_formatter{options};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "Test message with custom suffix",
                               nullptr,
                               LogLevel::Info,
                               MacroMetadata::Event::Log};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg), fmtquill::runtime(macro_metadata.message_format()));

  std::vector<std::pair<std::string, std::string>> named_args;

  auto const& formatted_buffer = custom_pattern_formatter.format(
    ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  // Convert the buffer to a string
  std::string const formatted_string = fmtquill::to_string(formatted_buffer);

  // Should have custom '|' suffix
  std::string const expected_string = "Test message with custom suffix|";
  REQUIRE_EQ(formatted_string, expected_string);
}

TEST_CASE("pattern_suffix_newline_trimming")
{
  // Test newline trimming behavior - should only trim when using newline suffix
  PatternFormatterOptions options_newline{"%(message)", "%H:%M:%S", Timezone::GmtTime, false, '\n'};
  PatternFormatter formatter_newline{options_newline};

  PatternFormatterOptions options_custom{"%(message)", "%H:%M:%S", Timezone::GmtTime, false, '|'};
  PatternFormatter formatter_custom{options_custom};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "Message with trailing newline{}",
                               nullptr,
                               LogLevel::Info,
                               MacroMetadata::Event::Log};

  // Create message with trailing newline
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg),
                      fmtquill::runtime(macro_metadata.message_format()), "\n");

  std::vector<std::pair<std::string, std::string>> named_args;

  // Test with newline suffix - should trim trailing newline from message
  auto const& formatted_buffer_newline = formatter_newline.format(
    ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
    std::string_view{log_msg.data(), log_msg.size()});

  std::string const formatted_string_newline = fmtquill::to_string(formatted_buffer_newline);
  std::string const expected_string_newline =
    "Message with trailing newline\n"; // Original \n trimmed, pattern \n added

  REQUIRE_EQ(formatted_string_newline, expected_string_newline);

  // Test with custom suffix - should NOT trim trailing newline from message
  auto const& formatted_buffer_custom =
    formatter_custom.format(ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata,
                            &named_args, std::string_view{log_msg.data(), log_msg.size()});

  std::string const formatted_string_custom = fmtquill::to_string(formatted_buffer_custom);
  std::string const expected_string_custom =
    "Message with trailing newline\n|"; // Original \n preserved, custom | added

  REQUIRE_EQ(formatted_string_custom, expected_string_custom);
}

TEST_CASE("pattern_suffix_multiline_with_custom_suffix")
{
  // Test multi-line formatting with custom suffix and trailing newline
  PatternFormatterOptions options_custom{"%(message)", "%H:%M:%S", Timezone::GmtTime, true, '|'};
  PatternFormatter formatter_custom{options_custom};

  PatternFormatterOptions options_newline{"%(message)", "%H:%M:%S", Timezone::GmtTime, true, '\n'};
  PatternFormatter formatter_newline{options_newline};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                               __func__,
                               "Line 1{}Line 2{}Line 3{}",
                               nullptr,
                               LogLevel::Info,
                               MacroMetadata::Event::Log};

  std::vector<std::pair<std::string, std::string>> named_args;

  // Test case 1: Multi-line message ending with newline (demonstrates different behavior)
  {
    fmtquill::memory_buffer log_msg;
    fmtquill::format_to(std::back_inserter(log_msg),
                        fmtquill::runtime(macro_metadata.message_format()), "\n", "\n", "\n");

    auto const& formatted_buffer_custom = formatter_custom.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
      std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_custom = fmtquill::to_string(formatted_buffer_custom);
    // Multi-line processing: finds \n in message to split lines, applies custom suffix to each
    // With non-newline suffix: trailing \n creates empty line that gets custom suffix
    std::string const expected_string_custom = "Line 1\n|Line 2\n|Line 3\n|";
    REQUIRE_EQ(formatted_string_custom, expected_string_custom);

    auto const& formatted_buffer_newline = formatter_newline.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
      std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_newline = fmtquill::to_string(formatted_buffer_newline);
    // With \n suffix: trailing \n in message is automatically skipped in multi-line loop
    std::string const expected_string_newline = "Line 1\nLine 2\nLine 3\n";
    REQUIRE_EQ(formatted_string_newline, expected_string_newline);
  }

  // Test case 2: Multi-line message NOT ending with newline
  {
    MacroMetadata macro_metadata_no_trailing{__FILE__ ":" QUILL_STRINGIFY(__LINE__),
                                             __func__,
                                             "Line 1{}Line 2{}Line 3",
                                             nullptr,
                                             LogLevel::Info,
                                             MacroMetadata::Event::Log};

    fmtquill::memory_buffer log_msg;
    fmtquill::format_to(std::back_inserter(log_msg),
                        fmtquill::runtime(macro_metadata_no_trailing.message_format()), "\n", "\n");

    auto const& formatted_buffer_custom = formatter_custom.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata_no_trailing,
      &named_args, std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_custom = fmtquill::to_string(formatted_buffer_custom);
    // Should process each line with custom suffix, last line is "Line 3"
    std::string const expected_string_custom = "Line 1\n|Line 2\n|Line 3|";
    REQUIRE_EQ(formatted_string_custom, expected_string_custom);

    auto const& formatted_buffer_newline = formatter_newline.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
      std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_newline = fmtquill::to_string(formatted_buffer_newline);
    // Should process each line with newline suffix, last line is "Line 3"
    std::string const expected_string_newline = "Line 1\nLine 2\nLine 3\n";
    REQUIRE_EQ(formatted_string_newline, expected_string_newline);
  }

  // Test case 3: Single line with trailing newline (tests the loop exit behavior)
  {
    MacroMetadata macro_metadata_single{
      __FILE__ ":" QUILL_STRINGIFY(__LINE__), __func__, "Single line{}", nullptr, LogLevel::Info, MacroMetadata::Event::Log};

    fmtquill::memory_buffer log_msg;
    fmtquill::format_to(std::back_inserter(log_msg),
                        fmtquill::runtime(macro_metadata_single.message_format()), "\n");

    auto const& formatted_buffer_custom = formatter_custom.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata_single,
      &named_args, std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_custom = fmtquill::to_string(formatted_buffer_custom);
    // Should process the line and the loop exits before processing the trailing \n
    std::string const expected_string_custom = "Single line\n|";
    REQUIRE_EQ(formatted_string_custom, expected_string_custom);

    auto const& formatted_buffer_newline = formatter_newline.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
      std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_newline = fmtquill::to_string(formatted_buffer_newline);
    // Should process each line with newline suffix, last line is "Line 3"
    std::string const expected_string_newline = "Single line\n";
    REQUIRE_EQ(formatted_string_newline, expected_string_newline);
  }
}

TEST_CASE("pattern_suffix_binary_data_preservation")
{
  // Test case similar to the user's binary data example
  PatternFormatterOptions options_nullopt{"%(message)", "%H:%M:%S", Timezone::GmtTime, false,
                                          PatternFormatterOptions::NO_SUFFIX};
  PatternFormatter formatter_nullopt{options_nullopt};

  PatternFormatterOptions options_newline{"%(message)", "%H:%M:%S", Timezone::GmtTime, false, '\n'};
  PatternFormatter formatter_newline{options_newline};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{
    __FILE__ ":" QUILL_STRINGIFY(__LINE__), __func__, "{}", nullptr, LogLevel::Critical, MacroMetadata::Event::Log};

  // Create binary-like data ending with 0x0a (\n)
  char const buffer[5] = {'\x0b', '\x0c', '\x0d', '\x0e', '\x0a'};
  std::string_view binary_data{buffer, 5};

  // Format to a buffer
  fmtquill::memory_buffer log_msg;
  fmtquill::format_to(std::back_inserter(log_msg), fmtquill::runtime(macro_metadata.message_format()), binary_data);

  std::vector<std::pair<std::string, std::string>> named_args;

  // Test with nullopt suffix - should preserve all 5 bytes
  auto const& formatted_buffer_nullopt = formatter_nullopt.format(
    ts, thread_id, thread_name, process_id, logger_name, "CRITICAL", "C", macro_metadata,
    &named_args, std::string_view{log_msg.data(), log_msg.size()});

  std::string const formatted_string_nullopt = fmtquill::to_string(formatted_buffer_nullopt);
  REQUIRE_EQ(formatted_string_nullopt.size(), 5); // All 5 bytes preserved
  REQUIRE_EQ(formatted_string_nullopt, std::string(buffer, 5));

  // Test with newline suffix - should trim trailing \n from message and add pattern \n
  auto const& formatted_buffer_newline = formatter_newline.format(
    ts, thread_id, thread_name, process_id, logger_name, "CRITICAL", "C", macro_metadata,
    &named_args, std::string_view{log_msg.data(), log_msg.size()});

  std::string const formatted_string_newline = fmtquill::to_string(formatted_buffer_newline);
  REQUIRE_EQ(formatted_string_newline.size(), 5); // 4 bytes + 1 pattern newline

  // First 4 bytes should be preserved, last byte should be pattern newline
  std::string expected_newline_result{buffer, 4};
  expected_newline_result += '\n';
  REQUIRE_EQ(formatted_string_newline, expected_newline_result);
}

TEST_CASE("pattern_suffix_multiline_metadata_with_no_suffix")
{
  // Test multi-line formatting with metadata enabled and NO_SUFFIX
  // This documents the ACTUAL BEHAVIOR: NO_SUFFIX changes multi-line processing significantly
  PatternFormatterOptions options_no_suffix{"%(message)", "%H:%M:%S", Timezone::GmtTime, true,
                                            PatternFormatterOptions::NO_SUFFIX};
  PatternFormatter formatter_no_suffix{options_no_suffix};

  uint64_t const ts{1579815761000023000};
  char const* thread_id = "31341";
  std::string const logger_name = "test_logger";
  MacroMetadata macro_metadata{
    __FILE__ ":" QUILL_STRINGIFY(__LINE__), __func__, "{}", nullptr, LogLevel::Info, MacroMetadata::Event::Log};

  std::vector<std::pair<std::string, std::string>> named_args;

  {
    fmtquill::memory_buffer log_msg;
    fmtquill::format_to(std::back_inserter(log_msg),
                        fmtquill::runtime(macro_metadata.message_format()), "Line1\nLine2\nLine3\n");

    auto const& formatted_buffer_no_suffix = formatter_no_suffix.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
      std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_no_suffix = fmtquill::to_string(formatted_buffer_no_suffix);

    // CORRECTED BEHAVIOR: With NO_SUFFIX, multi-line processing works but no suffix is added
    // Lines are processed separately but concatenated without newlines (NO_SUFFIX means no suffix)
    std::string const expected_no_suffix = "Line1\nLine2\nLine3\n";

    REQUIRE_EQ(formatted_string_no_suffix, expected_no_suffix);
  }

  {
    fmtquill::memory_buffer log_msg;
    fmtquill::format_to(std::back_inserter(log_msg),
                        fmtquill::runtime(macro_metadata.message_format()), "Line1Line2\nLine3\n");

    auto const& formatted_buffer_no_suffix = formatter_no_suffix.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
      std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_no_suffix = fmtquill::to_string(formatted_buffer_no_suffix);

    // CORRECTED BEHAVIOR: With NO_SUFFIX, multi-line processing works but no suffix is added
    // Lines are processed separately but concatenated without newlines (NO_SUFFIX means no suffix)
    std::string const expected_no_suffix = "Line1Line2\nLine3\n";

    REQUIRE_EQ(formatted_string_no_suffix, expected_no_suffix);
  }

  {
    fmtquill::memory_buffer log_msg;
    fmtquill::format_to(std::back_inserter(log_msg),
                        fmtquill::runtime(macro_metadata.message_format()), "Line1Line2Line3");

    auto const& formatted_buffer_no_suffix = formatter_no_suffix.format(
      ts, thread_id, thread_name, process_id, logger_name, "INFO", "I", macro_metadata, &named_args,
      std::string_view{log_msg.data(), log_msg.size()});

    std::string const formatted_string_no_suffix = fmtquill::to_string(formatted_buffer_no_suffix);

    // CORRECTED BEHAVIOR: With NO_SUFFIX, multi-line processing works but no suffix is added
    // Lines are processed separately but concatenated without newlines (NO_SUFFIX means no suffix)
    std::string const expected_no_suffix = "Line1Line2Line3";

    REQUIRE_EQ(formatted_string_no_suffix, expected_no_suffix);
  }
}

TEST_SUITE_END();
