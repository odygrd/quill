#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/JsonSink.h"

#include <utility>

/**
 * This example shows how to write structured JSON logs to a file with `JsonFileSink`.
 * It also demonstrates a hybrid logger that writes JSON to a file and a traditional
 * text format to the console at the same time.
 *
 * For JSON logging, use named placeholders in the format string such as
 * `{method}` and `{endpoint}`.
 */

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend

  // Create a json file for output
  auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonFileSink>(
    "example_json.log",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::None);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  // `JsonFileSink` produces its own structured output, so the normal text pattern is left empty
  // to avoid unnecessary formatting work.
  quill::Logger* json_logger = quill::Frontend::create_or_get_logger(
    "json_logger", std::move(json_sink),
    quill::PatternFormatterOptions{"", "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  for (int i = 0; i < 2; ++i)
  {
    LOG_INFO(json_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }

  // You can also create a logger that writes to both the JSON file and stdout, with
  // each sink using the format that makes sense for it.
  auto json_sink_2 = quill::Frontend::get_sink("example_json.log");
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console_sink_id_1");

  // Define a custom format pattern for console logging, which includes named arguments in the output.
  // If you prefer to omit named arguments from the log messages, you can remove the "[%(named_args)]" part.
  quill::PatternFormatterOptions console_log_pattern = quill::PatternFormatterOptions{
    "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<20) "
    "%(message) [%(named_args)]"};

  // Create a logger named "hybrid_logger" that writes to both a JSON sink and a console sink.
  // Note: The JSON sink uses its own internal format, so the custom format defined here
  // will only apply to the console output (via console_sink).
  quill::Logger* hybrid_logger = quill::Frontend::create_or_get_logger(
    "hybrid_logger", {std::move(json_sink_2), std::move(console_sink)}, console_log_pattern);

  for (int i = 2; i < 4; ++i)
  {
    LOG_INFO(hybrid_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }

  LOG_INFO(hybrid_logger, "Operation {name} completed with code {code}", "Update", 123,
           "Data synced successfully");
}
