#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <utility>

/**
 * This example showcases the usage of LOG_BACKTRACE macros. Log messages generated using these
 * macros are enqueued from the frontend to the backend. However, the backend will only log them if
 * a specific condition is met or if they are manually flushed using flush_backtrace().
 */
int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));
  logger->set_log_level(quill::LogLevel::Debug);

  // Enable the backtrace with a ring buffer capacity of 2 messages to get flushed when
  // a LOG_ERROR(...) or higher severity log message occurs via this logger.
  // Backtrace has to be enabled only once in the beginning before calling LOG_BACKTRACE(...) for the first time.
  logger->init_backtrace(2u, quill::LogLevel::Error);

  LOG_INFO(logger, "Begin example {}", 1);

  LOG_BACKTRACE(logger, "Backtrace log {}", 1);
  LOG_BACKTRACE(logger, "Backtrace log {}", 2);
  LOG_BACKTRACE(logger, "Backtrace log {}", 3);
  LOG_BACKTRACE(logger, "Backtrace log {}", 4);

  LOG_INFO(logger, "Backtrace is not flushed yet as we requested to flush on errors {}", 1);

  // log message with severity error - This will also flush the backtrace which has 2 messages
  LOG_ERROR(logger, "An error has happened, Backtrace is also flushed.");

  // The backtrace is flushed again after LOG_ERROR but in this case it is empty
  LOG_ERROR(logger,
            "Another second error has happened, backtrace is flushed again but it is now empty.");

  // Log more backtrace messages
  LOG_BACKTRACE(logger, "Another Backtrace log {}", 1);
  LOG_BACKTRACE(logger, "Another Backtrace log {}", 2);

  LOG_DEBUG(logger, "No backtrace logs yet");
  LOG_INFO(logger, "Manually flush backtrace");

  logger->flush_backtrace();
}
