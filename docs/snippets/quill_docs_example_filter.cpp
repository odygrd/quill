#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

int main()
{
  quill::Backend::start();

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  LOG_INFO(logger, "This is a log info example {}", 123);

  // Apply filter to the sink. This is thread-safe but to avoid setting the filter before the
  // backend has processed the earlier log message we flush first
  logger->flush_log();
  quill::Frontend::get_sink("sink_id_1")->set_log_level_filter(quill::LogLevel::Error);
  LOG_INFO(logger, "This message is filtered");

  logger->flush_log();
  quill::Frontend::get_sink("sink_id_1")->set_log_level_filter(quill::LogLevel::TraceL3);
  LOG_INFO(logger, "This is a log info example {}", 456);
}