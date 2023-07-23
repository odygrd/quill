#include "quill/Quill.h"

static char const* filename = "example_filehandler.log";

/**
 * Logging to the same file using multiple loggers
 */

void library_foo_create_logger_and_log()
{
  // Get a handler to the file
  // The first time this function is called a file handler is created for this filename.
  // Calling the function with the same filename will return the existing handler
  std::shared_ptr<quill::Handler> file_handler = quill::file_handler(filename,
                                                                     []()
                                                                     {
                                                                       quill::FileHandlerConfig cfg;
                                                                       cfg.set_open_mode('w');
                                                                       return cfg;
                                                                     }());

  // Create a logger using this handler
  quill::Logger* logger_foo = quill::create_logger("logger_foo", std::move(file_handler));

  LOG_INFO(logger_foo, "Hello from {}", "library foo");
}

void library_bar_create_logger_and_log()
{
  // Because foo already created the handler we will get a pointer to the existing handler
  std::shared_ptr<quill::Handler> file_handler = quill::file_handler(filename,
                                                                     []()
                                                                     {
                                                                       quill::FileHandlerConfig cfg;
                                                                       cfg.set_open_mode('w');
                                                                       return cfg;
                                                                     }());

  // Create a logger using this handler
  quill::Logger* logger_bar = quill::create_logger("logger_bar", std::move(file_handler));

  LOG_INFO(logger_bar, "Hello from {}", "library bar");
}

void library_foo_get_logger_and_log()
{
  // Library foo in a different place wants to obtain the logger and log
  auto logger = quill::get_logger("logger_foo");
  LOG_INFO(logger, "Hello again from library foo");
}

void library_bar_get_logger_and_log()
{
  // Library foo in a different place wants to obtain the logger and log
  auto logger = quill::get_logger("logger_bar");
  LOG_INFO(logger, "Hello again from library bar");
}

int main()
{
  // Start the logging backend thread
  quill::start();

  library_foo_create_logger_and_log();

  library_bar_create_logger_and_log();

  library_foo_get_logger_and_log();

  library_bar_get_logger_and_log();
}