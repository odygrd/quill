#include "quill/Quill.h"

/**
 * Trivial logging example
 */

int main()
{
  // Start the logging backend thread
  quill::start();

  {
    // Example 1:
    // Loggers can store in a ring buffer messages with QUILL_LOG_BACKTRACE and display later when
    // e.g. a LOG_ERROR message was logged from this logger

    quill::Logger* logger = quill::create_logger("example_1");

    // Enable the backtrace with a ring buffer capacity of 2 messages to get flushed when
    // a QUILL_LOG_ERROR(...) or higher severity log message occurs via this logger.
    // Backtrace has to be enabled only once in the beginning before calling QUILL_LOG_BACKTRACE(...) for the first time.
    logger->init_backtrace(2, quill::LogLevel::Error);

    QUILL_LOG_INFO(logger, "BEFORE backtrace Example {}", 1);

    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 1);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 2);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 3);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 4);

    // Backtrace is not flushed yet as we requested to flush on errors
    QUILL_LOG_INFO(logger, "AFTER backtrace Example {}", 1);

    // log message with severity error - This will also flush the backtrace which has 2 messages
    QUILL_LOG_ERROR(logger, "An error has happened, Backtrace is also flushed.");

    // The backtrace is flushed again after QUILL_LOG_ERROR but in this case it is empty
    QUILL_LOG_ERROR(logger, "An second error has happened, but backtrace is now empty.");

    // Log more backtrace messages
    QUILL_LOG_BACKTRACE(logger, "Another Backtrace log {}", 1);
    QUILL_LOG_BACKTRACE(logger, "Another Backtrace log {}", 2);

    // Nothing is logged at the moment
    QUILL_LOG_INFO(logger, "Another log info");

    // Still nothing logged - the error message is on a different logger object
    quill::Logger* logger_2 = quill::create_logger("example_1_1");
    QUILL_LOG_CRITICAL(logger_2, "A critical error from different logger.");

    // The new backtrace is flushed again due to QUILL_LOG_CRITICAL
    QUILL_LOG_CRITICAL(logger, "A critical error from the logger we had a backtrace.");
  }

  {
    // Example 2:
    // Loggers can store in a ring buffer messages with QUILL_LOG_BACKTRACE and display later on demand

    quill::Logger* logger = quill::create_logger("example_2");
    QUILL_LOG_INFO(logger, "BEFORE backtrace Example {}", 2);

    uint32_t backtrace_capacity = 2;
    logger->init_backtrace(backtrace_capacity);

    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 100);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 200);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 300);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 400);

    QUILL_LOG_INFO(logger, "AFTER backtrace Example {}", 2);

    // an error has happened - flush the backtrace manually
    logger->flush_backtrace();
  }

  {
    // Example 3:
    // Loggers can store in a ring buffer messages with QUILL_LOG_BACKTRACE and display later when
    // e.g. a LOG_ERROR message was logged from this logger

    quill::Logger* logger = quill::create_logger("example_3");
    QUILL_LOG_INFO(logger, "BEFORE backtrace Example {}", 3);

    uint32_t backtrace_capacity = 2;
    quill::LogLevel flush_backtrace = quill::LogLevel::Error;
    logger->init_backtrace(backtrace_capacity, flush_backtrace);

    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 1);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 2);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 3);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 4);

    QUILL_LOG_INFO(logger, "AFTER backtrace Example {}", 3);

    // No log error message - backtrace is never flushed
  }

  {
    // Example 4:
    // Loggers can store in a ring buffer messages with QUILL_LOG_BACKTRACE and display later on demand

    quill::Logger* logger = quill::create_logger("example_4");
    QUILL_LOG_INFO(logger, "BEFORE backtrace Example {}", 4);

    uint32_t backtrace_capacity = 2;
    logger->init_backtrace(backtrace_capacity);

    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 100);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 200);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 300);
    QUILL_LOG_BACKTRACE(logger, "Backtrace log {}", 400);

    QUILL_LOG_INFO(logger, "AFTER backtrace Example {}", 4);

    // no error has happened backtrace is never flushed
  }
}
