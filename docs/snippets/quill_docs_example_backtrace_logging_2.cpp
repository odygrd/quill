#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

int main()
{
  quill::Backend::start();

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "logger", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  // Store maximum of two log messages. By default, they will never be flushed since no LogLevel severity is specified
  logger->init_backtrace(2);

  LOG_INFO(logger, "BEFORE backtrace Example {}", 2);

  LOG_BACKTRACE(logger, "Backtrace log {}", 100);
  LOG_BACKTRACE(logger, "Backtrace log {}", 200);
  LOG_BACKTRACE(logger, "Backtrace log {}", 300);

  LOG_INFO(logger, "AFTER backtrace Example {}", 2);

  // flush the backtrace manually
  logger->flush_backtrace();
}