#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
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
TEST_CASE("json_malformed_named_arg_keeps_literal_message")
{
  static constexpr char const* filename = "json_malformed_named_arg_keeps_literal_message.json";

  BackendOptions bo;
  bo.error_notifier = [](std::string const&) {};
  Backend::start(bo);

  auto json_file_sink = Frontend::create_or_get_sink<CustomJsonFileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    "root_malformed_named_arg", std::move(json_file_sink),
    quill::PatternFormatterOptions{"LOG_%(log_level:<9) %(logger:<12) %(message) [%(named_args)]"});

  LOG_WARNING(logger, "Malformed named arg {param_1", 42);

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 1);

  std::string const expected_json_statement =
    R"({"logger":"root_malformed_named_arg","log_level":"WARNING","message":"Malformed named arg {param_1"})";

  REQUIRE_EQ(file_contents[0], expected_json_statement);

  testing::remove_file(filename);
}
