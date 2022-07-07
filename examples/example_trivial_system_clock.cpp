#include "quill/Quill.h"

/**
 * Trivial logging example
 */

int main()
{
  // Config using the custom ts class and the stdout handler
  quill::Config cfg;
  cfg.default_timestamp_clock_type = quill::TimestampClockType::System;
  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  // Using the default logger.
  // Default handler : stdout
  // Default LogLevel : Info
  // Default pattern : "%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - %(message)"
  quill::Logger* logger = quill::get_logger();

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
  LOG_INFO(logger, "This is a log info example {}", 5);
  LOG_WARNING(logger, "This is a log warning example {}", 6);
  LOG_ERROR(logger, "This is a log error example {}", 7);
  LOG_CRITICAL(logger, "This is a log critical example {}", 8);
}