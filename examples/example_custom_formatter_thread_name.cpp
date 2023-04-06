#include "quill/Quill.h"

// Just to set the thread name
#include "quill/detail/misc/Os.h"

/**
 * Logging to a file using a custom formatter pattern which also displays the thread name
 */

int main()
{
  // Get the stdout file handler
  std::shared_ptr<quill::Handler> file_handler = quill::stdout_handler();

  // Set a custom formatter for this handler
  file_handler->set_pattern(
    "%(ascii_time) [%(process)] [%(thread)] [%(thread_name)] %(logger_name) - %(message)", // format
    "%D %H:%M:%S.%Qms %z",     // timestamp format
    quill::Timezone::GmtTime); // timestamp's timezone

  // set the default logger's handler to be the new handler with the custom format string
  quill::Config cfg;
  cfg.default_handlers.emplace_back(file_handler);
  quill::configure(cfg);

  // Start the backend logging thread
  // quill::set_default_logger_handler MUST be called before the backend worker thread starts
  quill::start();

  // Before issuing any log statement we MUST set the thread name. The thread name is cached after
  // the first log statement
  // Here the internal function to set the thread name is used but any function that sets the thread
  // name will work
  quill::detail::set_thread_name("MainThread");

  // Log using the default logger
  LOG_INFO(quill::get_logger(), "The default logger is using a custom format");

  std::thread t1([]() {
    // Must set the thread name before any log statement
    quill::detail::set_thread_name("NewThread");

    // Obtain a new logger. Since no handlers were specified during the creation of the new logger. The new logger will use the default logger's handlers. In that case it will use the stdout_handler with the modified format.
    quill::Logger* logger_foo = quill::create_logger("logger_foo");

    LOG_INFO(logger_foo, "The new logger is using the custom format");

    // Backtrace log
    logger_foo->init_backtrace(2, quill::LogLevel::Error);
    LOG_BACKTRACE(logger_foo, "Backtrace log {}", 1);
    LOG_BACKTRACE(logger_foo, "Backtrace log {}", 2);
    LOG_ERROR(logger_foo, "An error has happened, Backtrace is also flushed.");
  });
  t1.join();

  // Log using the default logger
  LOG_INFO(quill::get_logger(), "Done");
}