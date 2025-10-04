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
 * This example demonstrates how to create a RotatingJsonFileSink with daily rotation and automatic
 * rotation based on maximum file size, providing your own custom json formatting
 * For additional configuration options, refer to RotatingFileSinkConfig.
 */

class CustomJsonSink : public quill::RotatingJsonFileSink
{
public:
  using quill::RotatingJsonFileSink::RotatingJsonFileSink;

  void generate_json_message(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp,
                             std::string_view thread_id, std::string_view /** thread_name **/,
                             std::string const& /** process_id **/, std::string_view logger_name,
                             quill::LogLevel /** log_level **/, std::string_view log_level_description,
                             std::string_view /** log_level_short_code **/,
                             std::vector<std::pair<std::string, std::string>> const* named_args,
                             std::string_view /** log_message **/,
                             std::string_view /** log_statement **/, char const* message_format) override
  {
    // e.g. custom time formatting
    std::time_t log_timestamp_seconds = log_timestamp / 1'000'000'000;

    // Convert to localtime
    std::tm* tm = std::localtime(&log_timestamp_seconds);

    // Format the time as YYYY-MM-DD HH:MM:SS
    char formatted_time[20];
    std::strftime(formatted_time, sizeof(formatted_time), "%Y-%m-%d %H:%M:%S", tm);

    // format json as desired
    _json_message.append(fmtquill::format(
      R"({{"timestamp":"{}","file_name":"{}","line":"{}","thread_id":"{}","logger":"{}","log_level":"{}","message":"{}")",
      formatted_time, log_metadata->file_name(), log_metadata->line(), thread_id, logger_name,
      log_level_description, message_format));

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