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
    LOG_INFO(logger, "BEFORE backtrace Example {}", 1);

    uint32_t backtrace_capacity = 2;
    quill::LogLevel flush_backtrace = quill::LogLevel::Error;
    logger->enable_backtrace(backtrace_capacity, flush_backtrace);

    LOG_BACKTRACE(logger, "Backtrace log {}", 1);
    LOG_BACKTRACE(logger, "Backtrace log {}", 2);
    LOG_BACKTRACE(logger, "Backtrace log {}", 3);
    LOG_BACKTRACE(logger, "Backtrace log {}", 4);

    LOG_INFO(logger, "AFTER backtrace Example {}", 1);

    // log message with severity error - This will also flush the backtrace
    LOG_ERROR(logger, "An error has happened, Backtrace is also flushed.");
    LOG_ERROR(logger, "An second error has happened, but backtrace is now empty.");
  }

  {
    // Example 2:
    // Loggers can store in a ring buffer messages with LOG_BACKTRACE and display later on demand

    quill::Logger* logger = quill::create_logger("example_2");
    LOG_INFO(logger, "BEFORE backtrace Example {}", 2);

    uint32_t backtrace_capacity = 2;
    logger->enable_backtrace(backtrace_capacity);

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

    uint32_t backtrace_capacity = 2;
    quill::LogLevel flush_backtrace = quill::LogLevel::Error;
    logger->enable_backtrace(backtrace_capacity, flush_backtrace);

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

    uint32_t backtrace_capacity = 2;
    logger->enable_backtrace(backtrace_capacity);

    LOG_BACKTRACE(logger, "Backtrace log {}", 100);
    LOG_BACKTRACE(logger, "Backtrace log {}", 200);
    LOG_BACKTRACE(logger, "Backtrace log {}", 300);
    LOG_BACKTRACE(logger, "Backtrace log {}", 400);

    LOG_INFO(logger, "AFTER backtrace Example {}", 4);

    // no error has happened backtrace is never flushed
  }
}