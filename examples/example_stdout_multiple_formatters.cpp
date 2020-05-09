#include "quill/Quill.h"

/**
 * Logging to a file using a custom formatter pattern
 */

int main()
{
  // Start the backend logging thread
  quill::start();

  // Get the stdout file handler
  quill::Handler* stdout_handler_1 = quill::stdout_handler("stdout_1");

  // Set a custom formatter for this handler
  stdout_handler_1->set_pattern(
    QUILL_STRING(
      "%(ascii_time) [%(process)] [%(thread)] LOG_%(level_name) %(logger_name) - %(message)"), // log recorder format
    "%D %H:%M:%S.%Qms %z",     // timestamp format
    quill::Timezone::GmtTime); // timestamp's timezone

  // Obtain a new logger. Since no handlers were specified during the creation of the new logger. The new logger will use the default logger's handlers. In that case it will use the stdout_handler with the modified format.
  quill::Logger* logger_foo = quill::create_logger("logger_foo", stdout_handler_1);

  // Get the stdout file handler
  quill::Handler* stdout_handler_2 = quill::stdout_handler("stdout_2");

  // Set a custom formatter for this handler
  stdout_handler_2->set_pattern(
    QUILL_STRING("%(ascii_time) LOG_%(level_name) %(logger_name) - %(message)"), // log recorder format
    "%D %H:%M:%S.%Qms %z",                                                       // timestamp format
    quill::Timezone::GmtTime); // timestamp's timezone

  // Obtain a new logger. Since no handlers were specified during the creation of the new logger. The new logger will use the default logger's handlers. In that case it will use the stdout_handler with the modified format.
  quill::Logger* logger_bar = quill::create_logger("logger_bar", stdout_handler_2);

  // Use both loggers
  LOG_INFO(logger_foo, "The logger is using stdout_handler_1");
  LOG_INFO(logger_bar, "The logger is using stdout_handler_2");
  LOG_INFO(logger_bar, "Logging from {}", "logger_bar");
  LOG_INFO(logger_foo, "Logging from {}", "logger_foo");
}