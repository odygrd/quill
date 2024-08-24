#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"

#include "ConsoleSinkWithFormatter.h"
#include "quill/sinks/RotatingFileSink.h"

/**
 * This example demonstrates how to use a single Logger to output different formats for each Sink.
 * While the Logger class accepts a single format pattern, you can override this by creating a custom Sink with your own format pattern.
 */
int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Console sink
  std::string console_log_pattern =
    "%(time) [PID %(process_id)] [%(log_level)] [%(logger)] - %(message)";
  std::string console_time_format = "%Y-%m-%d %H:%M:%S.%Qms";
  auto console_sink = quill::Frontend::create_or_get_sink<ConsoleSinkWithFormatter>(
    "sink_id_1", quill::PatternFormatterOptions{console_log_pattern, console_time_format});
  console_sink->set_log_level_filter(quill::LogLevel::Warning);

  // File sink
  std::string const file_log_pattern = "%(log_level);%(time);%(logger);%(message)";
  std::string const file_time_format = "%Y%m%dT%H:%M:%S.%Qus";

  auto rotating_file_sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
    "rotating_file.log",
    []()
    {
      // See RotatingFileSinkConfig for more options
      quill::RotatingFileSinkConfig cfg;
      cfg.set_open_mode('a');
      cfg.set_max_backup_files(10);
      cfg.set_rotation_max_file_size(1024 * 1024);
      return cfg;
    }());
  rotating_file_sink->set_log_level_filter(quill::LogLevel::Info);

  // The Logger is using the file_log_pattern by default
  // To output our custom format to the file we use our own ConsoleSinkWithFormatter that is
  // overwriting the default format
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", {std::move(console_sink), std::move(rotating_file_sink)},
    quill::PatternFormatterOptions{file_log_pattern, file_time_format});

  logger->set_log_level(quill::LogLevel::Debug);

  LOG_INFO(logger, "This is a log info example {}", sizeof(std::string));
  LOG_WARNING(logger, "This is a log warning example {}", sizeof(std::string));
  LOG_ERROR(logger, "This is a log error example {}", sizeof(std::string));
  LOG_CRITICAL(logger, "This is a log critical example {}", sizeof(std::string));
  return 0;
}