#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/StringRef.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("string_no_copy_logging")
{
  static constexpr char const* filename = "string_no_copy_logging.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

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

  static std::string s = "adipiscing";
  static std::string_view sv = "string_view";
  static char const* c_style_string_empty = "";
  static const char* c_style_string = "Lorem ipsum";
  static const char* npcs = "Example\u0003String\u0004";

  std::string s1 = "adipiscing_1";
  char const* s2 = "adipiscing_2";

  LOG_INFO(logger, "static string [{}]", quill::utility::StringRef{s});
  LOG_INFO(logger, "static string_view [{}]", quill::utility::StringRef{sv});
  LOG_INFO(logger, "static c_style_string_empty [{}]", quill::utility::StringRef{c_style_string_empty});
  LOG_INFO(logger, "static c_style_string [{}]", quill::utility::StringRef{c_style_string});
  LOG_INFO(logger, "static npcs [{}]", quill::utility::StringRef{npcs});
  LOG_INFO(logger, "string literal [{}]", quill::utility::StringRef{"test string literal"});
  LOG_INFO(logger, "mix strings [{}] [{}] [{}] [{}]",
           quill::utility::StringRef{"test string literal"}, s1, quill::utility::StringRef{s}, s2);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 7);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       static string [adipiscing]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       static string_view [string_view]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       static c_style_string_empty []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       static c_style_string [Lorem ipsum]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       static npcs [Example\\x03String\\x04]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       string literal [test string literal]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       mix strings [test string literal] [adipiscing_1] [adipiscing] [adipiscing_2]"}));

  testing::remove_file(filename);
}