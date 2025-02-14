#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

int main()
{
  quill::Backend::start();

  quill::Logger* logger_1 = quill::Frontend::create_or_get_logger(
    "logger_1", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  quill::Logger* logger_2 = quill::Frontend::create_or_get_logger(
    "logger_2", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  // a LOG_ERROR(...) or higher severity log message occurs via this logger.
  // Enable the backtrace with a max ring buffer size of 2 messages which will get flushed when
  // Backtrace has to be enabled only once in the beginning before calling LOG_BACKTRACE(...) for the first time.
  logger_1->init_backtrace(2, quill::LogLevel::Error);

  LOG_INFO(logger_1, "BEFORE backtrace Example {}", 1);

  LOG_BACKTRACE(logger_1, "Backtrace log {}", 1);
  LOG_BACKTRACE(logger_1, "Backtrace log {}", 2);
  LOG_BACKTRACE(logger_1, "Backtrace log {}", 3);
  LOG_BACKTRACE(logger_1, "Backtrace log {}", 4);

  // Backtrace is not flushed yet as we requested to flush on errors
  LOG_INFO(logger_1, "AFTER backtrace Example {}", 1);

  // log message with severity error - This will also flush_sink the backtrace which has 2 messages
  LOG_ERROR(logger_1, "An error has happened, Backtrace is also flushed.");

  // The backtrace is flushed again after LOG_ERROR but in this case it is empty
  LOG_ERROR(logger_1, "An second error has happened, but backtrace is now empty.");

  // Log more backtrace messages
  LOG_BACKTRACE(logger_1, "Another Backtrace log {}", 1);
  LOG_BACKTRACE(logger_1, "Another Backtrace log {}", 2);

  // Nothing is logged at the moment
  LOG_INFO(logger_1, "Another log info");

  // Still nothing logged - the error message is on a different logger object
  LOG_CRITICAL(logger_2, "A critical error from different logger.");

  // The new backtrace is flushed again due to LOG_CRITICAL
  LOG_CRITICAL(logger_1, "A critical error from the logger we had a backtrace.");
}