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

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(file_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                          "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  // set the log level of the logger to debug (default is info)
  logger->set_log_level(quill::LogLevel::Debug);

  LOG_INFO(logger, "log something {}", 123);
  LOG_DEBUG(logger, "something else {}", 456);

  log_from_new_logger();
  log_from_existing_logger();
}