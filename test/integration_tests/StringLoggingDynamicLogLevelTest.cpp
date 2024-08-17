#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("string_logging_dynamic_log_level")
{
  static constexpr char const* filename = "string_logging_dynamic_log_level.log";
  static std::string const logger_name = "logger";
  static constexpr size_t number_of_messages = 1000;

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

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));
  logger->set_log_level(LogLevel::TraceL3);

  {
    std::string s = "adipiscing";
    std::string const empty_s{};

    std::string_view begin_s{"begin_s"};
    std::string_view const end_s{"end_s"};
    std::string_view empty_sv{};

    char const* c_style_string_empty = "";
    const char* c_style_string = "Lorem ipsum";

    char c_style_char_array_empty[] = "";
    char const c_style_char_array[] = "dolor";

    char c_style_string_array_non_terminated[3];
    c_style_string_array_non_terminated[0] = 'A';
    c_style_string_array_non_terminated[1] = 'B';
    c_style_string_array_non_terminated[2] = 'C';

    LOG_DYNAMIC(logger, LogLevel::TraceL3, "s [{}]", s);
    LOG_CRITICAL(logger, "s [{}]", s);
    LOG_DYNAMIC(logger, LogLevel::TraceL2, "empty_s [{}]", empty_s);
    LOG_ERROR(logger, "empty_s [{}]", empty_s);
    LOG_DYNAMIC(logger, LogLevel::TraceL1, "begin_s [{}]", begin_s);
    LOG_WARNING(logger, "begin_s [{}]", begin_s);
    LOG_DYNAMIC(logger, LogLevel::Debug, "end_s [{}]", end_s);
    LOG_DYNAMIC(logger, LogLevel::Info, "empty_sv [{}]", empty_sv);
    LOG_DYNAMIC(logger, LogLevel::Warning, "c_style_string_empty [{}]", c_style_string_empty);
    LOG_DYNAMIC(logger, LogLevel::Error, "c_style_string [{}]", c_style_string);
    LOG_DYNAMIC(logger, LogLevel::Critical, "c_style_char_array_empty [{}]", c_style_char_array_empty);
    LOG_DYNAMIC(logger, LogLevel::Info, "c_style_char_array [{}]", c_style_char_array);
    LOG_DYNAMIC(logger, LogLevel::Info, "c_style_string_array_non_terminated [{}]",
                c_style_string_array_non_terminated);

    LOG_DYNAMIC(
      logger, LogLevel::Info,
      "Lorem ipsum dolor sit amet, consectetur [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] "
      "[{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}]",
      s, "elit", 1, 3.14, empty_s, begin_s, end_s, empty_sv, c_style_string_empty, c_style_string,
      c_style_char_array_empty, c_style_char_array, c_style_string_array_non_terminated, s, "elit",
      1, 3.14, empty_s, begin_s, end_s, empty_sv, c_style_string_empty, c_style_string,
      c_style_char_array_empty, c_style_char_array, c_style_string_array_non_terminated);

    LOG_DYNAMIC(logger, LogLevel::Error,
                "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante [{}] [{}] "
                "[{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] "
                "[{}] [{}] [{}] [{}]",
                2, true, begin_s, empty_sv, empty_s, c_style_string_array_non_terminated, c_style_string_empty,
                c_style_string, end_s, c_style_char_array_empty, c_style_char_array, 2, true,
                begin_s, empty_sv, empty_s, c_style_string_array_non_terminated, c_style_string_empty,
                c_style_string, end_s, c_style_char_array_empty, c_style_char_array);
  }

  // Log a big string
  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string v{"Lorem ipsum dolor sit amet, consectetur "};
    v += std::to_string(i);

    LOG_DYNAMIC(logger, LogLevel::Info, "Logging int: {}, int: {}, string: {}, char: {}", i, i * 10,
                v, v.c_str());
  }

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string v{"Lorem ipsum dolor sit amet, consectetur "};
    v += std::to_string(i);

    LOG_CRITICAL(logger, "Logging int: {}, int: {}, string: {}", i, i * 10, v);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), (number_of_messages * 2) + 15);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L3  " + logger_name + "       s [adipiscing]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_CRITICAL  " + logger_name + "       s [adipiscing]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L2  " + logger_name + "       empty_s []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     " + logger_name + "       empty_s []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L1  " + logger_name + "       begin_s [begin_s]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_WARNING   " + logger_name + "       begin_s [begin_s]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_DEBUG     " + logger_name + "       end_s [end_s]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empty_sv []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_WARNING   " + logger_name + "       c_style_string_empty []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     " + logger_name + "       c_style_string [Lorem ipsum]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_CRITICAL  " + logger_name + "       c_style_char_array_empty []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c_style_char_array [dolor]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c_style_string_array_non_terminated [ABC]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Lorem ipsum dolor sit amet, consectetur [adipiscing] [elit] [1] [3.14] [] [begin_s] [end_s] [] [] [Lorem ipsum] [] [dolor] [ABC] [adipiscing] [elit] [1] [3.14] [] [begin_s] [end_s] [] [] [Lorem ipsum] [] [dolor] [ABC]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     " + logger_name + "       Nulla tempus, libero at dignissim viverra, lectus libero finibus ante [2] [true] [begin_s] [] [] [ABC] [] [Lorem ipsum] [end_s] [] [dolor] [2] [true] [begin_s] [] [] [ABC] [] [Lorem ipsum] [end_s] [] [dolor]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Logging int: 0, int: 0, string: Lorem ipsum dolor sit amet, consectetur 0, char: Lorem ipsum dolor sit amet, consectetur 0"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Logging int: 999, int: 9990, string: Lorem ipsum dolor sit amet, consectetur 999, char: Lorem ipsum dolor sit amet, consectetur 999"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_CRITICAL  " + logger_name + "       Logging int: 0, int: 0, string: Lorem ipsum dolor sit amet, consectetur 0"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_CRITICAL  " + logger_name + "       Logging int: 999, int: 9990, string: Lorem ipsum dolor sit amet, consectetur 999"}));

  testing::remove_file(filename);
}