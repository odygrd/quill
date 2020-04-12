#include "quill/Quill.h"

/**
 * Logging to a file using a custom formatter pattern
 */

int main()
{
  // Start the backend logging thread
  quill::start();

  // Get the stdout handler
  quill::Handler* file_handler = quill::stdout_handler();

  // Set a custom formatter to the handler
  file_handler->set_pattern(QUILL_STRING("%(ascii_time) %(logger_name) - %(message)"),
                            "%D %H:%M:%S", quill::PatternFormatter::TimestampPrecision::MilliSeconds,
                            quill::PatternFormatter::Timezone::GmtTime);

  // This line sets the default logger's handler to be the new handler with the custom format string
  quill::set_default_logger_handler(file_handler);

  // Log using the default logger
  LOG_INFO(quill::get_logger(), "The default logger is using a custom format");

  // Obtain a new logger. Since no handlers were specified during the creation of the new logger. The new logger will use the default logger's handlers. In that case it will use the stdout_handler with the modified format.
  quill::Logger* logger_foo = quill::create_logger("logger_foo");

  LOG_INFO(logger_foo, "The new logger is using the custom format");
}