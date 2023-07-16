#include "quill/Quill.h"

/**
 * Logging to the same file using multiple loggers
 */

int main()
{
  // Get or create a handler to the file
  // quill::FilenameAppend::DateTime will append the start date and time to the provided filename
  std::shared_ptr<quill::Handler> file_handler =
    quill::file_handler("example_trivial.log", "w", quill::FilenameAppend::DateTime);

  // Set a custom formatter for this handler
  file_handler->set_pattern("%(ascii_time) [%(process)] [%(thread)] %(logger_name) - %(message)", // format
                            "%D %H:%M:%S.%Qms %z",     // timestamp format
                            quill::Timezone::GmtTime); // timestamp's timezone

  quill::Config cfg;

  // Add the filehandler as a default
  cfg.default_handlers.push_back(std::move(file_handler));

  // Apply the configuration
  quill::configure(cfg);

  // Start the backend logging thread
  quill::start();

  // Any logger created will inherit the properties of the default_handler we set above
  quill::Logger* logger_1 = quill::create_logger("logger_1");
  quill::Logger* logger_2 = quill::create_logger("logger_2");

  QUILL_LOG_INFO(logger_1, "log something {}", 123);
  QUILL_LOG_INFO(logger_2, "something else {}", 456);
}