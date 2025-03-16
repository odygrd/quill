#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"

#include "quill/sinks/ConsoleSink.h"

/**
 * This example demonstrates how to use a single Logger to output different formats for each Sink.
 * While the Logger class accepts a single format pattern, you can override this by creating a custom Sink with your own format pattern.
 */
int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Console sink 1 - uses logger formatter
  auto console_sink_1 = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // Console sink 2 - verrides formatter
  auto console_sink_2 = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
    "sink_id_2",
    []()
    {
      quill::ConsoleSinkConfig csc;
      csc.set_override_pattern_formatter_options(quill::PatternFormatterOptions{
        "%(time) [PID %(process_id)] [%(log_level)] [%(logger)] - %(message)"});
      return csc;
    }());

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", {std::move(console_sink_1), std::move(console_sink_2)});

  logger->set_log_level(quill::LogLevel::Debug);

  LOG_INFO(logger, "This is a log info example {}", sizeof(std::string));
  LOG_WARNING(logger, "This is a log warning example {}", sizeof(std::string));
  LOG_ERROR(logger, "This is a log error example {}", sizeof(std::string));
  LOG_CRITICAL(logger, "This is a log critical example {}", sizeof(std::string));
  return 0;
}