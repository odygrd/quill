#include "quill/Quill.h"

/**
 * Logging to the same file using multiple loggers
 */

int main()
{
  // Get or create a handler to the file
  // quill::FilenameAppend::DateTime will append the start date and time to the provided filename
  std::shared_ptr<quill::Handler> file_handler =
    quill::file_handler("example_trivial.log",
                        []()
                        {
                          quill::FileHandlerConfig cfg;
                          cfg.set_open_mode('w');
                          cfg.set_append_to_filename(quill::FilenameAppend::StartDateTime);
                          return cfg;
                        }());

  // Optional - Set a custom formatter for this handler, or skip this line to use the default one
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

  // Log using the root logger
  quill::Logger* root_logger = quill::get_logger();
  QUILL_LOG_INFO(root_logger, "log something {}", 123);

  // Any logger created will inherit the properties of the default_handler we set above
  quill::Logger* logger_1 = quill::create_logger("logger_1");
  quill::Logger* logger_2 = quill::create_logger("logger_2");

  QUILL_LOG_INFO(logger_1, "log something {}", 123);
  QUILL_LOG_INFO(logger_2, "something else {}", 456);
}