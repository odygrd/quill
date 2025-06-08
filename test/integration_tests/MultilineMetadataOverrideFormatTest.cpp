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
TEST_CASE("multi_line_metadata_override_format")
{
  static constexpr char const* filename_a = "multi_line_metadata_override_format_a.log";
  static constexpr char const* filename_b = "multi_line_metadata_override_format_b.log";
  static std::string const logger_name = "logger_a";

  // Start the logging backend thread
  Backend::start();

  Frontend::preallocate();

  // Set writing logging to a file
  auto file_sink_a = Frontend::create_or_get_sink<FileSink>(
    filename_a,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');

      PatternFormatterOptions options;
      options.format_pattern =
        "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) "
        "%(message) %(caller_function)";
      options.timestamp_pattern = "%H:%M:%S.%Qns";
      options.timestamp_timezone = Timezone::LocalTime;
      options.add_metadata_to_multi_line_logs = false;
      cfg.set_override_pattern_formatter_options(options);

      return cfg;
    }(),
    FileEventNotifier{});

  auto file_sink_b = Frontend::create_or_get_sink<FileSink>(
    filename_b,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      // no override format - has multi line header
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger_a = Frontend::create_or_get_logger(
    logger_name, {file_sink_a, file_sink_b},
    PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                            "LOG_%(log_level:<9) %(logger:<12) %(message) %(caller_function)",
                            "%H:%M:%S.%Qns", Timezone::LocalTime, true});

  LOG_INFO(logger_a, "This is a multiline info message.\nLine 2: {}.\nLine 3: {}.", "data3",
           "data4");
  LOG_WARNING(logger_a, "Warning: Multiple issues detected:\n1. {}.\n2. {}.\n3. {}.", "issue4",
              "issue5", "issue6");
  LOG_INFO(logger_a, "Another multiline info message [{}]\nLine 2: [{}]\n", "data1", "data2");
  LOG_INFO(logger_a, "");
  LOG_INFO(logger_a, "End");

  logger_a->flush_log();
  Frontend::remove_logger(logger_a);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_a);
    REQUIRE_EQ(file_contents.size(), 11);

    std::string expected_string = logger_name + "     This is a multiline info message.";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "Line 2: data3.";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "Line 3: data4.";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     Warning: Multiple issues detected:";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "1. issue4.";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "2. issue5.";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "3. issue6. DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     Another multiline info message [data1]";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = "Line 2: [data2] DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "      DOCTEST_ANON_FUNC_2"; // empty message
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     End";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_b);
    REQUIRE_EQ(file_contents.size(), 11);

    std::string expected_string =
      logger_name + "     This is a multiline info message. DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     Line 2: data3. DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     Line 3: data4. DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     Warning: Multiple issues detected: DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     1. issue4. DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     2. issue5. DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     3. issue6. DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string =
      logger_name + "     Another multiline info message [data1] DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     Line 2: [data2] DOCTEST_ANON_FUNC_2";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "      DOCTEST_ANON_FUNC_2"; // empty message
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));

    expected_string = logger_name + "     End";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  testing::remove_file(filename_a);
  testing::remove_file(filename_b);
}