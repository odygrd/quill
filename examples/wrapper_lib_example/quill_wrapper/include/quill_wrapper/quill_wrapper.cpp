#include "quill_wrapper.h"

#include "quill/Quill.h"

// Define a global variable for a logger to avoid looking up the logger each time.
// Additional global variables can be defined for additional loggers if needed.
quill::Logger* global_logger_a;

void setup_quill(char const* log_file)
{
  // Get or create a handler to the file
  // quill::FilenameAppend::DateTime will append the start date and time to the provided filename
  std::shared_ptr<quill::Handler> file_handler =
    quill::file_handler(log_file,
                        []()
                        {
                          quill::FileHandlerConfig cfg;
                          cfg.set_open_mode('w');
                          cfg.set_append_to_filename(quill::FilenameAppend::StartDateTime);
                          return cfg;
                        }());

  // Optional - Set a custom formatter for this handler, or skip this line to use the default one
  file_handler->set_pattern("%(time) [%(process_id)] [%(thread_id)] %(logger) - %(message)", // format
                            "%D %H:%M:%S.%Qms %z",     // timestamp format
                            quill::Timezone::GmtTime); // timestamp's timezone

  quill::Config cfg;

  // Add the filehandler as a default
  cfg.default_handlers.push_back(std::move(file_handler));

  // Apply the configuration
  quill::configure(cfg);

  // Start the backend logging thread
  quill::start();

  global_logger_a = quill::get_logger();
}