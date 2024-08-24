#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/JsonFileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("json_multi_line_metadata")
{
  static constexpr char const* filename = "json_multi_line_metadata.log";
  static constexpr char const* json_filename = "json_multi_line_metadata.json";
  static std::string const logger_name_a = "logger_a";
  static std::string const logger_name_b = "logger_b";

  // Start the logging backend thread
  Backend::start();

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

  auto json_file_sink = Frontend::create_or_get_sink<JsonFileSink>(
    json_filename,
    []()
    {
      JsonFileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger_a = Frontend::create_or_get_logger(
    logger_name_a, std::initializer_list<std::shared_ptr<Sink>>{json_file_sink, file_sink},
    PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) "
      "%(message) %(caller_function)",
      "%H:%M:%S.%Qns", Timezone::LocalTime, true});

  Logger* logger_b = Frontend::create_or_get_logger(
    logger_name_b, std::initializer_list<std::shared_ptr<Sink>>{json_file_sink, file_sink},
    PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) "
      "%(message) %(caller_function)",
      "%H:%M:%S.%Qns", Timezone::LocalTime, false});

  LOG_INFO(logger_a, "This is a multiline info message.\nLine 2: {val_1}.\nLine 3: {val_2}.",
           "data1", "data2");
  LOG_WARNING(logger_a, "Warning: Multiple issues detected:\n1. {val_1}.\n2. {val_2}.\n3. {val_3}.",
              "issue1", "issue2", "issue3");
  LOG_INFO(logger_a, "Another multiline info message.\nLine 2: {val_1}\n", "data1", "data2");

  LOG_INFO(logger_b, "This is a multiline info message.\nLine 2: {val_1}.\nLine 3: {val_2}.",
           "data3", "data4");
  LOG_WARNING(logger_b, "Warning: Multiple issues detected:\n1. {val_1}.\n2. {val_2}.\n3. {val_3}.",
              "issue4", "issue5", "issue6");
  LOG_INFO(logger_b, "Another multiline info message.\nLine 2: {val_1}\n", "data1", "data2");

  logger_a->flush_log();
  Frontend::remove_logger(logger_a);
  Frontend::remove_logger(logger_b);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 18);

  std::string expected_string = logger_name_a + "     This is a multiline info message.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "Line 2: data1.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "Line 3: data2. DOCTEST_ANON_FUNC_2";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = logger_name_a + "     Warning: Multiple issues detected:";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "1. issue1.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "2. issue2.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "3. issue3. DOCTEST_ANON_FUNC_2";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = logger_name_a + "     Another multiline info message.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "Line 2: data1.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = logger_name_b + "     This is a multiline info message.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "Line 2: data3.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "Line 3: data4.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = logger_name_b + "     Warning: Multiple issues detected:";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "1. issue4.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "2. issue5.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "3. issue6. DOCTEST_ANON_FUNC_2";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = logger_name_b + "     Another multiline info message.";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  expected_string = "Line 2: data DOCTEST_ANON_FUNC_2";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string));

  std::vector<std::string> const file_contents_json = quill::testing::file_contents(json_filename);
  REQUIRE_EQ(file_contents_json.size(), 6);

  expected_string = R"("logger":"logger_a","log_level":"INFO","message":"This is a multiline info message. Line 2: {val_1}. Line 3: {val_2}.","val_1":"data1","val_2":"data2"})";
  REQUIRE(quill::testing::file_contains(file_contents_json, expected_string));

  expected_string = R"("logger":"logger_b","log_level":"INFO","message":"Another multiline info message. Line 2: {val_1} ","val_1":"data1"})";
  REQUIRE(quill::testing::file_contains(file_contents_json, expected_string));

  testing::remove_file(filename);
  testing::remove_file(json_filename);
}