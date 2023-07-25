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
    // Loggers can store in a ring buffer messages with LOG_BACKTRACE and display later when e.g.
    // a LOG_ERROR message was logged from this logger

    quill::Logger* logger = quill::create_logger("example_1");

    // Enable the backtrace with a ring buffer capacity of 2 messages to get flushed when
    // a LOG_ERROR(...) or higher severity log message occurs via this logger.
    // Backtrace has to be enabled only once in the beginning before calling LOG_BACKTRACE(...) for the first time.
    logger->init_backtrace(2u, quill::LogLevel::Error);

    LOG_INFO(logger, "BEFORE backtrace Example {}", 1);

    LOG_BACKTRACE(logger, "Backtrace log {}", 1);
    LOG_BACKTRACE(logger, "Backtrace log {}", 2);
    LOG_BACKTRACE(logger, "Backtrace log {}", 3);
    LOG_BACKTRACE(logger, "Backtrace log {}", 4);

    // Backtrace is not flushed yet as we requested to flush on errors
    LOG_INFO(logger, "AFTER backtrace Example {}", 1);

    // log message with severity error - This will also flush the backtrace which has 2 messages
    LOG_ERROR(logger, "An error has happened, Backtrace is also flushed.");

    // The backtrace is flushed again after LOG_ERROR but in this case it is empty
    LOG_ERROR(logger, "An second error has happened, but backtrace is now empty.");

    // Log more backtrace messages
    LOG_BACKTRACE(logger, "Another Backtrace log {}", 1);
    LOG_BACKTRACE(logger, "Another Backtrace log {}", 2);

    // Nothing is logged at the moment
    LOG_INFO(logger, "Another log info");

    // Still nothing logged - the error message is on a different logger object
    quill::Logger* logger_2 = quill::create_logger("example_1_1");
    LOG_CRITICAL(logger_2, "A critical error from different logger.");

    // The new backtrace is flushed again due to LOG_CRITICAL
    LOG_CRITICAL(logger, "A critical error from the logger we had a backtrace.");
  }

  {
    // Example 2:
    // Loggers can store in a ring buffer messages with LOG_BACKTRACE and display later on demand

    quill::Logger* logger = quill::create_logger("example_2");
    LOG_INFO(logger, "BEFORE backtrace Example {}", 2);

    uint32_t backtrace_capacity = 2u;
    logger->init_backtrace(backtrace_capacity);

    LOG_BACKTRACE(logger, "Backtrace log {}", 100);
    LOG_BACKTRACE(logger, "Backtrace log {}", 200);
    LOG_BACKTRACE(logger, "Backtrace log {}", 300);
    LOG_BACKTRACE(logger, "Backtrace log {}", 400);

    LOG_INFO(logger, "AFTER backtrace Example {}", 2);

    // an error has happened - flush the backtrace manually
    logger->flush_backtrace();
  }

  {
    // Example 3:
    // Loggers can store in a ring buffer messages with LOG_BACKTRACE and display later when e.g.
    // a LOG_ERROR message was logged from this logger

    quill::Logger* logger = quill::create_logger("example_3");
    LOG_INFO(logger, "BEFORE backtrace Example {}", 3);

    uint32_t backtrace_capacity = 2u;
    quill::LogLevel flush_backtrace = quill::LogLevel::Error;
    logger->init_backtrace(backtrace_capacity, flush_backtrace);

    LOG_BACKTRACE(logger, "Backtrace log {}", 1);
    LOG_BACKTRACE(logger, "Backtrace log {}", 2);
    LOG_BACKTRACE(logger, "Backtrace log {}", 3);
    LOG_BACKTRACE(logger, "Backtrace log {}", 4);

    LOG_INFO(logger, "AFTER backtrace Example {}", 3);

    // No log error message - backtrace is never flushed
  }

  {
    // Example 4:
    // Loggers can store in a ring buffer messages with LOG_BACKTRACE and display later on demand

    quill::Logger* logger = quill::create_logger("example_4");
    LOG_INFO(logger, "BEFORE backtrace Example {}", 4);

    uint32_t backtrace_capacity = 2u;
    logger->init_backtrace(backtrace_capacity);

    LOG_BACKTRACE(logger, "Backtrace log {}", 100);
    LOG_BACKTRACE(logger, "Backtrace log {}", 200);
    LOG_BACKTRACE(logger, "Backtrace log {}", 300);
    LOG_BACKTRACE(logger, "Backtrace log {}", 400);

    LOG_INFO(logger, "AFTER backtrace Example {}", 4);

    // no error has happened backtrace is never flushed
  }
}