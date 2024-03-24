#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <string>
#include <utility>

/**
 * Trivial logging example to console
 */

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
  LOG_INFO(logger, "This is a log info example {}", sizeof(std::string));
  LOG_WARNING(logger, "This is a log warning example {}", sizeof(std::string));
  LOG_ERROR(logger, "This is a log error example {}", sizeof(std::string));
  LOG_CRITICAL(logger, "This is a log critical example {}", sizeof(std::string));

  // libfmt format specification mini language is supported
  // note: named arguments are not supported
  LOG_INFO(logger, "Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
  LOG_INFO(logger, "Easy padding in numbers like {:08d}", 12);
  LOG_INFO(logger, "{:>30}", "right aligned");
  LOG_INFO(logger, "Positional arguments {1} {2} {0} ", "too", "are", "supported");
  LOG_INFO(logger, "Support for precision {:.4f}", 1.23456);

  // To log with a different format to the same sink, just create another logger
  auto console_sink_2 = quill::Frontend::get_sink("sink_id_1"); // get the created sink
  quill::Logger* logger_2 = quill::Frontend::create_or_get_logger(
    "logger_2", std::move(console_sink_2), "%(time) %(log_level:<9) %(logger:<12) %(message)");

  logger_2->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(logger_2, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger_2, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger_2, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(logger_2, "This is a log debug example {}", 4);
  LOG_INFO(logger_2, "This is a log info example {}", sizeof(std::string));
  LOG_WARNING(logger_2, "This is a log warning example {}", sizeof(std::string));
  LOG_ERROR(logger_2, "This is a log error example {}", sizeof(std::string));
  LOG_CRITICAL(logger_2, "This is a log critical example {}", sizeof(std::string));
}
