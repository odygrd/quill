#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/RotatingJsonFileSink.h"

#include <ctime>
#include <utility>

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4996)
#endif

/**
 * This example shows a custom `RotatingJsonFileSink` that keeps the normal
 * rotation behaviour but overrides how each JSON record is generated.
 *
 * See `RotatingFileSinkConfig` for additional rotation options.
 */

class CustomJsonSink : public quill::RotatingJsonFileSink
{
public:
  using quill::RotatingJsonFileSink::RotatingJsonFileSink;

  void generate_json_message(quill::LogRecord const& record, char const* message_format) override
  {
    // e.g. custom time formatting
    std::time_t log_timestamp_seconds = record.log_timestamp / 1'000'000'000;

    // Convert to localtime
    std::tm* tm = std::localtime(&log_timestamp_seconds);

    // Format the time as YYYY-MM-DD HH:MM:SS
    char formatted_time[20];
    std::strftime(formatted_time, sizeof(formatted_time), "%Y-%m-%d %H:%M:%S", tm);

    // format json as desired; escape any text that can contain quotes, backslashes or control characters
    _json_message.append(fmtquill::format(R"({{"timestamp":"{}","file_name":")", formatted_time));
    append_json_escaped(_json_message, record.log_metadata->file_name());
    _json_message.append(fmtquill::format(R"(","line":"{}","thread_id":"{}","logger":")",
                                          record.log_metadata->line(), record.thread_id));
    append_json_escaped(_json_message, record.logger_name);
    _json_message.append(fmtquill::format(R"(","log_level":"{}","message":")", record.log_level_description));
    append_json_escaped(_json_message, message_format);
    _json_message.append(std::string_view{"\""});

    // add log statement arguments as key-values to the json
    if (record.named_args)
    {
      for (auto const& [key, value] : *record.named_args)
      {
        _json_message.append(std::string_view{",\""});
        append_json_escaped(_json_message, key);
        _json_message.append(std::string_view{"\":\""});
        append_json_escaped(_json_message, value);
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
  auto rotating_json_sink = quill::Frontend::create_or_get_sink<CustomJsonSink>(
    "rotating_json.log",
    []()
    {
      // See RotatingFileSinkConfig for more options
      quill::RotatingFileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
      cfg.set_rotation_time_daily("18:30");
      cfg.set_rotation_max_file_size(1024); // small value to demonstrate the example
      return cfg;
    }());

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(rotating_json_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                   "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  for (int i = 0; i < 20; ++i)
  {
    LOG_INFO(logger, "Hello from rotating logger {index}", i);
  }
}

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif
