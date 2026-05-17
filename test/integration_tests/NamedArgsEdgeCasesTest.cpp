#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/sinks/FileSink.h"
#include "quill/sinks/JsonSink.h"

#include <string>
#include <vector>

using namespace quill;

class CustomJsonFileSink : public JsonFileSink
{
public:
  using JsonFileSink::JsonFileSink;

  void write_log(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                 std::string_view thread_id, std::string_view thread_name,
                 std::string const& process_id, std::string_view logger_name, LogLevel log_level,
                 std::string_view log_level_description, std::string_view log_level_short_code,
                 std::vector<std::pair<std::string, std::string>> const* named_args,
                 std::string_view, std::string_view) override
  {
    _json_message.clear();

    _json_message.append(fmtquill::format(R"({{"logger":"{}","log_level":"{}","message":"{}")", logger_name,
                                          log_level_description, log_metadata->message_format()));

    if (named_args)
    {
      for (auto const& [key, value] : *named_args)
      {
        _json_message.append(std::string_view{",\""});
        _json_message.append(key);
        _json_message.append(std::string_view{"\":\""});
        _json_message.append(value);
        _json_message.append(std::string_view{"\""});
      }
    }

    _json_message.append(std::string_view{"}\n"});

    FileSink::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id, logger_name,
                        log_level, log_level_description, log_level_short_code, named_args,
                        std::string_view{}, std::string_view{_json_message.data(), _json_message.size()});
  }

private:
  fmtquill::memory_buffer _json_message;
};

/***/
TEST_CASE("named_args_edge_cases")
{
  static constexpr char const* json_filename = "named_args_edge_cases.json";
  static constexpr char const* text_filename = "named_args_edge_cases.log";

  std::vector<std::string> error_messages;

  BackendOptions bo;
  bo.error_notifier = [&error_messages](std::string const& error_message)
  { error_messages.push_back(error_message); };
  Backend::start(bo);

  auto json_file_sink = Frontend::create_or_get_sink<CustomJsonFileSink>(
    json_filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    text_filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    "root_named_args_edge_cases",
    std::initializer_list<std::shared_ptr<Sink>>{std::move(json_file_sink), std::move(file_sink)},
    quill::PatternFormatterOptions{"LOG_%(log_level:<9) %(logger:<12) %(message) [%(named_args)]"});

  char const separator_payload_bytes[] = {'A', '\x01', '\x02', '\x03', 'B'};
  std::string const separator_payload{separator_payload_bytes, sizeof(separator_payload_bytes)};

  // 1. Malformed: unclosed brace — no named arg extracted, format error
  LOG_WARNING(logger, "Malformed named arg {param_1", 42);
  // 2. Named arg followed by literal } via }}} — should work
  LOG_INFO(logger, "{name}}} text", "value");
  // 3. Named arg followed by }} — unmatched } in output format string, format error
  LOG_INFO(logger, "{name}} text", "value");
  // 4. Escaped {{ and }} around literal text, then a named arg — should work
  LOG_INFO(logger, "text {{literal}} {name}", "value");
  // 5. Named arg with format specifier — should work
  LOG_INFO(logger, "{name:.2f} done", 3.14159);
  // 6. Multiple named args — should work
  LOG_INFO(logger, "{first} and {second}", "aaa", "bbb");
  // 7. Escaped {{ at start, named arg, escaped }} at end — should work
  LOG_INFO(logger, "{{prefix {name} suffix}}", "val");
  // 8. Named arg with dynamic width from a following arg — should work
  LOG_INFO(logger, ">{name:{width}}<", 12, 5);
  // 9. Named arg value containing Quill's old separator bytes — should remain intact
  LOG_INFO(logger, "{name}", separator_payload);
  // 10. Adjacent named args with no text in between
  LOG_INFO(logger, "{a}{b}{c}", 1, 2, 3);
  // 11. Named arg with fill, align, and width format spec
  LOG_INFO(logger, "[{name:*>10}]", "hi");
  // 12. Three named args with mixed types
  LOG_INFO(logger, "{s} {i} {d}", std::string_view{"text"}, 42, 3.14);
  // 13. Named arg with dynamic width and precision (two nested replacement fields)
  LOG_INFO(logger, "|{val:{width}.{precision}}|", 3.14159, 10, 4);
  // 14. Only escaped braces, no real named args — parser should produce no keys
  LOG_INFO(logger, "nothing {{here}} at all");
  // 15. Single named arg alone, no surrounding text
  LOG_INFO(logger, "{x}", 99);
  // 16. Named arg with hex format specifier
  LOG_INFO(logger, "{val:#x}", 255);

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  std::vector<std::string> const json_file_contents = quill::testing::file_contents(json_filename);
  std::vector<std::string> const text_file_contents = quill::testing::file_contents(text_filename);

  REQUIRE_EQ(json_file_contents.size(), 16u);
  REQUIRE_EQ(text_file_contents.size(), 16u);
  REQUIRE_EQ(error_messages.size(), 2u);

  // Check the malformed case (1)
  std::string const expected_json_statement =
    R"({"logger":"root_named_args_edge_cases","log_level":"WARNING","message":"Malformed named arg {param_1"})";
  REQUIRE_EQ(json_file_contents[0], expected_json_statement);

  REQUIRE(quill::testing::file_contains(error_messages, R"(message: "Malformed named arg {param_1")"));
  REQUIRE(quill::testing::file_contains(error_messages, R"(message: "{name}} text")"));

  // Error cases appear in text output as [Could not format ...]
  REQUIRE(quill::testing::file_contains(
    text_file_contents, R"([Could not format log statement. message: "Malformed named arg {param_1")"));
  REQUIRE(quill::testing::file_contains(
    text_file_contents, R"([Could not format log statement. message: "{name}} text")"));

  // Successful cases (2, 4, 5, 6, 7, 8, 9)
  REQUIRE(quill::testing::file_contains(text_file_contents, "value} text [name: value]"));
  REQUIRE(quill::testing::file_contains(text_file_contents, "text {literal} value [name: value]"));
  REQUIRE(quill::testing::file_contains(text_file_contents, "3.14 done [name: 3.14]"));
  REQUIRE(
    quill::testing::file_contains(text_file_contents, "aaa and bbb [first: aaa, second: bbb]"));
  REQUIRE(quill::testing::file_contains(text_file_contents, "{prefix val suffix} [name: val]"));
  REQUIRE(quill::testing::file_contains(text_file_contents, ">   12< [name:    12, _1: 5]"));
  REQUIRE(quill::testing::file_contains(text_file_contents, R"(A\x01\x02\x03B [name: A\x01\x02\x03B])"));

  // Adjacent named args (10)
  REQUIRE(quill::testing::file_contains(text_file_contents, "123 [a: 1, b: 2, c: 3]"));

  // Fill/align/width format spec (11)
  REQUIRE(quill::testing::file_contains(text_file_contents, "[********hi] [name: ********hi]"));

  // Three args with mixed types (12)
  REQUIRE(
    quill::testing::file_contains(text_file_contents, "text 42 3.14 [s: text, i: 42, d: 3.14]"));

  // Dynamic width and precision with two nested replacement fields (13)
  REQUIRE(quill::testing::file_contains(text_file_contents,
                                        "|     3.142| [val:      3.142, _1: 10, _2: 4]"));

  // Only escaped braces, no named args (14)
  REQUIRE(quill::testing::file_contains(text_file_contents, "nothing {here} at all []"));

  // Single named arg alone (15)
  REQUIRE(quill::testing::file_contains(text_file_contents, "99 [x: 99]"));

  // Hex format specifier (16)
  REQUIRE(quill::testing::file_contains(text_file_contents, "0xff [val: 0xff]"));

  testing::remove_file(json_filename);
  testing::remove_file(text_filename);
}
