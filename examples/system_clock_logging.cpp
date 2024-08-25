#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <string>
#include <utility>

/**
 * Trivial logging example to console using std::chrono::now for timestamps instead of the
 * default rdtsc clock
 */

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    quill::PatternFormatterOptions{"%(time) [%(process_id)] [%(thread_id)] %(logger) - %(message)",
                                   "%D %H:%M:%S.%Qms %z", quill::Timezone::GmtTime, false},
    quill::ClockSourceType::System);

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
}
