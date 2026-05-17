#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/JsonSink.h"

#include <string>
#include <vector>

using namespace quill;

/**
 * Regression test: U+2028 (LINE SEPARATOR) and U+2029 (PARAGRAPH SEPARATOR) must be
 * escaped as \\u2028 / \\u2029 in JSON output. They are valid JSON characters but break
 * JavaScript consumers that treat the JSON as JS source.
 */
TEST_CASE("json_line_separator_escape")
{
  static constexpr char const* json_filename = "json_line_separator_escape.json";
  static std::string const logger_name = "json_line_separator_escape_logger";

  BackendOptions bo;
  // Disable the default printable-char sanitizer so the raw UTF-8 bytes reach
  // the JSON sink and we can verify _append_json_escaped() handles them.
  bo.check_printable_char = {};
  Backend::start(bo);

  auto json_file_sink = Frontend::create_or_get_sink<JsonFileSink>(
    json_filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(json_file_sink),
                                                  quill::PatternFormatterOptions{"%(message)"});

  // U+2028 in UTF-8 is E2 80 A8, U+2029 is E2 80 A9
  std::string const line_sep{"\xE2\x80\xA8"};
  std::string const para_sep{"\xE2\x80\xA9"};
  std::string const both = "before" + line_sep + "middle" + para_sep + "after";

  // U+20AC (EURO SIGN) = E2 82 AC. Starts with E2 but is NOT U+2028/U+2029 and must
  // pass through unchanged.
  std::string const euro{"\xE2\x82\xAC"};

  // Truncated E2 (E2 with no follow-up bytes) - must not crash and must pass through.
  std::string const truncated_e2{"tail\xE2"};

  LOG_INFO(logger, "line {sep}", line_sep);
  LOG_INFO(logger, "para {sep}", para_sep);
  LOG_INFO(logger, "both {sep}", both);
  LOG_INFO(logger, "euro {sep}", euro);
  LOG_INFO(logger, "trunc {sep}", truncated_e2);

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(json_filename);
  REQUIRE_EQ(file_contents.size(), 5u);

  // Each line must contain the escaped form \\u2028 / \\u2029 in the output.
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"\"sep\":\"\\u2028\""}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"\"sep\":\"\\u2029\""}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"\"sep\":\"before\\u2028middle\\u2029after\""}));

  // The euro sign (E2 82 AC) is NOT one of the special separators - it must pass
  // through unchanged.
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"\"sep\":\"\xE2\x82\xAC\""}));

  // A trailing lone E2 must pass through unchanged (no crash, no escape).
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"\"sep\":\"tail\xE2\""}));

  // And critically: the raw U+2028/U+2029 byte sequences must NOT appear in the output.
  for (auto const& line : file_contents)
  {
    REQUIRE_EQ(line.find("\xE2\x80\xA8"), std::string::npos);
    REQUIRE_EQ(line.find("\xE2\x80\xA9"), std::string::npos);
  }

  testing::remove_file(json_filename);
}
