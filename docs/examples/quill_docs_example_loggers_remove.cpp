#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"

#include <string>

// Stored logger pointer
quill::Logger* g_logger{nullptr};

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Store created logger
  std::string const logger_name{"root"};
  g_logger = quill::Frontend::create_or_get_logger(
    logger_name, quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  LOG_INFO(g_logger, "A {} message with number {}", "log", 123);

  // Remove the logger
  // total_loggers can help you check the logger is actually removed, as long as you are not
  // creating/removing other loggers from different threads simultaneously
  g_logger->flush_log();
  size_t const total_loggers = quill::Frontend::get_number_of_loggers();
  quill::Frontend::remove_logger(g_logger);

  // remove_logger is async, after this call logger is invalidated but still exists,
  // we want to wait until it is actually removed by the Backend
  // quill::Frontend::get_number_of_loggers() is the only function that will also take invalidated loggers into account
  while (quill::Frontend::get_number_of_loggers() != (total_loggers - 1))
  {
    // wait
  }

  // Make sure you also update all references to the previous logger* (e.g. if stored as class
  // member) as the previous is now invalid after removal
  g_logger = quill::Frontend::create_or_get_logger(
    logger_name, quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"),
    quill::PatternFormatterOptions{"%(time) LOG_%(log_level:<9) %(message)"});

  LOG_INFO(g_logger, "A second {} message with number {}", 456, "log");
}