#include "quill/Quill.h"

/**
 * Logging to a file using a custom formatter pattern
 */

int main()
{
  // Get the stdout file handler
  std::shared_ptr<quill::Handler> file_handler = quill::stdout_handler();

  // Set a custom formatter for this handler
  file_handler->set_pattern("%(ascii_time) [%(process)] [%(thread)] %(logger_name) - %(message)", // format
                            "%D %H:%M:%S.%Qms %z",     // timestamp format
                            quill::Timezone::GmtTime); // timestamp's timezone

  // Config using the custom ts class and the stdout handler
  quill::Config cfg;
  cfg.default_handlers.emplace_back(file_handler);
  quill::configure(cfg);

  // Start the backend logging thread
  quill::start();

  // Log using the default logger
  LOG_INFO(quill::get_logger(), "The default logger is using a custom format");

  // Obtain a new logger. Since no handlers were specified during the creation of the new logger. The new logger will use the default logger's handlers. In that case it will use the stdout_handler with the modified format.
  quill::Logger* logger_foo = quill::create_logger("logger_foo");

  LOG_INFO(logger_foo, "The new logger is using the custom format");
}