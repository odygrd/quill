#include "quill/Quill.h"

/**
 * Example on how to create a new stdout handler with console colours or
 * customise the console colours.
 */

int main()
{
  // Enable console colours on the default constructed handler to stdout before calling quill:start()
  quill::enable_console_colours();

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