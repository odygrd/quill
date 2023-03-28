#include "quill/Quill.h"

/**
 * Logging to stdout using multiple custom formatter patterns
 */
int main()
{
  quill::start();

  // Get the stdout file handler, with a unique name
  std::shared_ptr<quill::Handler> stdout_handler_1 = quill::stdout_handler("stdout_1");

  stdout_handler_1->set_pattern(
    "%(ascii_time) [%(process)] [%(thread)] LOG_%(level_name) %(logger_name) - %(message)", // message format
    "%D %H:%M:%S.%Qms %z",     // timestamp format
    quill::Timezone::GmtTime); // timestamp's timezone

  quill::Logger* logger_foo = quill::create_logger("logger_foo", std::move(stdout_handler_1));

  // Get the stdout file handler, with another unique name
  std::shared_ptr<quill::Handler> stdout_handler_2 = quill::stdout_handler("stdout_2");

  stdout_handler_2->set_pattern("%(ascii_time) LOG_%(level_name) %(logger_name) - %(message)", // message format
                                "%D %H:%M:%S.%Qms %z",     // timestamp format
                                quill::Timezone::GmtTime); // timestamp's timezone

  quill::Logger* logger_bar = quill::create_logger("logger_bar", std::move(stdout_handler_2));

  // Use both loggers
  LOG_INFO(logger_foo, "The logger is using stdout_handler_1");
  LOG_INFO(logger_bar, "The logger is using stdout_handler_2");
  LOG_INFO(logger_bar, "Logging from {}", "logger_bar");
  LOG_INFO(logger_foo, "Logging from {}", "logger_foo");

  // Retrieve existing handler to create a new logger that will use the retrieved handler's format pattern.
  std::shared_ptr<quill::Handler> stdout_handler_3 = quill::stdout_handler("stdout_1");
  quill::Logger* logger_foo_2 = quill::create_logger("logger_foo_2", std::move(stdout_handler_3));
  LOG_INFO(logger_foo_2, "The logger is using stdout_handler_1");
  LOG_INFO(logger_foo_2, "Logging from {}", "logger_foo_2");
}