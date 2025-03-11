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
  // Note:: remove_logger(g_logger) is not enough for this example. We must wait until the logger
  // is removed in order to re-create one with the same name (logger_name)
  quill::Frontend::remove_logger_blocking(g_logger);

  // Recreate a logger with the same name (logger_name)
  // Make sure you also update all references to the previous logger* (e.g. if stored as class
  // member) as the previous is now invalid after removal
  g_logger = quill::Frontend::create_or_get_logger(
    logger_name, quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"),
    quill::PatternFormatterOptions{"%(time) LOG_%(log_level:<9) %(message)"});

  LOG_INFO(g_logger, "A second {} message with number {}", 456, "log");
}