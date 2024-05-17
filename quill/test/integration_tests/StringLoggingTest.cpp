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
TEST_CASE("string_logging")
{
  static constexpr char const* filename = "string_logging.log";
  static std::string const logger_name = "logger";
  static constexpr size_t number_of_messages = 10000;

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

  {
    std::string s = "adipiscing";
    std::string const& scr = s;
    std::string& sr = s;
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

    LOG_INFO(logger, "s [{}]", s);
    LOG_INFO(logger, "scr [{}]", scr);
    LOG_INFO(logger, "sr [{}]", sr);
    LOG_INFO(logger, "empty_s [{}]", empty_s);
    LOG_INFO(logger, "begin_s [{}]", begin_s);
    LOG_INFO(logger, "end_s [{}]", end_s);
    LOG_INFO(logger, "empty_sv [{}]", empty_sv);
    LOG_INFO(logger, "c_style_string_empty [{}]", c_style_string_empty);
    LOG_INFO(logger, "c_style_string [{}]", c_style_string);
    LOG_INFO(logger, "c_style_char_array_empty [{}]", c_style_char_array_empty);
    LOG_INFO(logger, "c_style_char_array [{}]", c_style_char_array);
    LOG_INFO(logger, "c_style_string_array_non_terminated [{}]", c_style_string_array_non_terminated);

    LOG_INFO(logger,
             "Lorem ipsum dolor sit amet, consectetur [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] "
             "[{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}]",
             s, "elit", 1, 3.14, empty_s, begin_s, end_s, empty_sv, c_style_string_empty, c_style_string,
             c_style_char_array_empty, c_style_char_array, c_style_string_array_non_terminated, s,
             "elit", 1, 3.14, empty_s, begin_s, end_s, empty_sv, c_style_string_empty, c_style_string,
             c_style_char_array_empty, c_style_char_array, c_style_string_array_non_terminated);

    LOG_ERROR(logger,
              "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante [{}] [{}] "
              "[{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] [{}] "
              "[{}] [{}] [{}] [{}]",
              2, true, begin_s, empty_sv, empty_s, c_style_string_array_non_terminated, c_style_string_empty,
              c_style_string, end_s, c_style_char_array_empty, c_style_char_array, 2, true, begin_s,
              empty_sv, empty_s, c_style_string_array_non_terminated, c_style_string_empty,
              c_style_string, end_s, c_style_char_array_empty, c_style_char_array);
  }

  // Log a big string
  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string v{"Lorem ipsum dolor sit amet, consectetur "};
    v += std::to_string(i);

    LOG_INFO(logger, "Logging int: {}, int: {}, string: {}, char: {}", i, i * 10, v, v.c_str());
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages + 14);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       s [adipiscing]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       scr [adipiscing]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sr [adipiscing]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empty_s []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       begin_s [begin_s]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       end_s [end_s]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       empty_sv []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c_style_string_empty []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c_style_string [Lorem ipsum]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       c_style_char_array_empty []"}));

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
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Logging int: 1999, int: 19990, string: Lorem ipsum dolor sit amet, consectetur 1999, char: Lorem ipsum dolor sit amet, consectetur 1999"}));

  testing::remove_file(filename);
}