#include "quill/Quill.h"

/**
 * Logging to a file using a custom formatter pattern
 */

int main()
{
  // Start the backend logging thread
  quill::start();

  // Create a file handler
  quill::Handler* file_handler = quill::stdout_handler();

  // Set a custom formatter to the handler
  file_handler->set_pattern(QUILL_STRING("%(ascii_time) %(logger_name) - %(message)"), "%D %H:%M:%S",
                            quill::PatternFormatter::TimestampPrecision::MilliSeconds);

  // Replacing the default logger a logger that is using a custom formatter
  quill::set_default_logger_handler(file_handler);

  // Log using the default logger
  LOG_INFO(quill::get_logger(), "The default logger is using a custom format");

  // Obtain a new logger, the new logger will use the format of the default logger
  quill::Logger* logger_foo = quill::create_logger("logger_foo");

  LOG_INFO(logger_foo, "The new logger is using the custom format");
}