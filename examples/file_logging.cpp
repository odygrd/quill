#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <utility>

/**
 * Logging to the same file using multiple loggers
 */

void log_from_new_logger()
{
  // Obtain the existing created sink, and create a new logger to log
  auto file_sink = quill::Frontend::get_sink("example_file_logging.log");
  quill::Logger* logger_2 = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

  LOG_INFO(logger_2, "log from new logger {}", 123);
}

void log_from_existing_logger()
{
  // Obtain existing logger to log
  quill::Logger* logger = quill::Frontend::get_logger("root");
  LOG_INFO(logger, "log again {}", 123312);
}

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    "example_file_logging.log",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::PatternFormatterOptions pfo;
  pfo.format_pattern =
    "%(time) [%(thread_id)] %(source_location:<28) "
    "LOG_%(log_level:<9) %(logger:<12) %(message)";
  pfo.timestamp_pattern = ("%H:%M:%S.%Qns");
  pfo.timestamp_timezone = quill::Timezone::GmtTime;
  pfo.source_location_path_strip_prefix = "quill";

  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink), pfo);

  // set the log level of the logger to debug (default is info)
  logger->set_log_level(quill::LogLevel::Debug);

#ifndef NDEBUG
  // In debug mode, force immediate flushing of log messages
  // This blocks the caller thread until log entry is written to disk
  // Simulates synchronous logging behavior for easier debugging
  // Note: This can impact performance and should only be used during development
  logger->set_immediate_flush(1u);
#endif

  LOG_INFO(logger, "log something {}", 123);
  LOG_DEBUG(logger, "something else {}", 456);

  log_from_new_logger();
  log_from_existing_logger();
}