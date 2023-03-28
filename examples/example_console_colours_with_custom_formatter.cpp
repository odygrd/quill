#include "quill/Quill.h"

/**
 * Example on how to create a new stdout handler with console colours or
 * customise the console colours.
 */

int main()
{
  // Get the stdout file handler
  std::shared_ptr<quill::Handler> stdout_handler = quill::stdout_handler();

  // Set a custom formatter for this handler
  stdout_handler->set_pattern("%(ascii_time) [%(process)] [%(thread)] %(logger_name) - %(message)", // format
                              "%Y-%m-%d %H:%M:%S.%Qms",  // timestamp format
                              quill::Timezone::GmtTime); // timestamp's timezone

  // Enable console colours on the handler
  static_cast<quill::ConsoleHandler*>(stdout_handler.get())->enable_console_colours();

  // Config using the custom ts class and the stdout handler
  quill::Config cfg;
  cfg.default_handlers.emplace_back(stdout_handler);
  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  quill::Logger* logger = quill::get_logger();

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // We also use backtrace
  logger->init_backtrace(2, quill::LogLevel::Critical);
  LOG_BACKTRACE(logger, "Backtrace log {}", 1);
  LOG_BACKTRACE(logger, "Backtrace log {}", 2);

  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger, "This is a log trace l1 example {}", 3);
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
  LOG_INFO(logger, "This is a log info example {}", 5);
  LOG_WARNING(logger, "This is a log warning example {}", 6);
  LOG_ERROR(logger, "This is a log error example {}", 7);
  LOG_CRITICAL(logger, "This is a log critical example {}", 8);
}