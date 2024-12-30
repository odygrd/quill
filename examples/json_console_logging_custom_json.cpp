#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/JsonSink.h"

#include <string>

/**
 * Overrides generate_json_message to use a custom json format
 */
class MyJsonConsoleSink : public quill::JsonConsoleSink
{
  void generate_json_message(quill::MacroMetadata const* /** log_metadata **/, uint64_t log_timestamp,
                             std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                             std::string const& /** process_id **/, std::string_view /** logger_name **/,
                             quill::LogLevel /** log_level **/, std::string_view log_level_description,
                             std::string_view /** log_level_short_code **/,
                             std::vector<std::pair<std::string, std::string>> const* named_args,
                             std::string_view /** log_message **/,
                             std::string_view /** log_statement **/, char const* message_format) override
  {
    // format json as desired
    _json_message.append(fmtquill::format(R"({{"timestamp":"{}","log_level":"{}","message":"{}")",
                                          std::to_string(log_timestamp), log_level_description, message_format));

    // add log statement arguments as key-values to the json
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
  }
};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend

  // Create a json sink
  auto json_sink = quill::Frontend::create_or_get_sink<MyJsonConsoleSink>("json_sink_1");

  // PatternFormatter is only used for non-structured logs formatting
  // When logging only json, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "json_logger", std::move(json_sink),
    quill::PatternFormatterOptions{"", "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  int var_a = 123;
  std::string var_b = "test";

  // Log via the convenient LOGJ_ macros
  LOGJ_INFO(logger, "A json message", var_a, var_b);

  // Or manually specify the desired names of each variable
  LOG_INFO(logger, "A json message with {var_1} and {var_2}", var_a, var_b);
}
