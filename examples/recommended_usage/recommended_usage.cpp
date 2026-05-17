/**
 * This example demonstrates the recommended Quill integration style for larger projects.
 *
 * Build your Quill setup code into your own small static library once, then link that library
 * into the rest of the application. Keep `Backend`, `Frontend`, sink creation, and logger
 * creation inside that wrapper library.
 *
 * This example exposes one global logger purely as a simple convenience. That is not a Quill
 * requirement. Your wrapper can instead expose logger getter functions, return logger pointers
 * from setup code, store loggers in objects, or manage multiple named loggers.
 *
 * Multiple loggers are often a better fit for larger applications because they allow different
 * subsystems to use different names, sinks, and log levels.
 *
 * In application code that only logs, include only the lightweight headers you need:
 *
 * - For logging:
 *   #include "quill/Logger.h"
 *   #include "quill/LogMacros.h"
 *
 * - For logger lookup or creation when needed:
 *   #include "quill/Frontend.h"
 *
 * - For sink creation when needed:
 *   #include "quill/sinks/.."
 */

// Include our wrapper lib
#include "quill_static.h"

// We need only those two headers in order to log
#include "quill/LogMacros.h"
#include "quill/Logger.h"

// This example uses a global logger exported by the wrapper library only to keep the example
// small. In real projects you can instead look up loggers with Frontend, expose getter
// functions from the wrapper library, or store logger pointers as members.
extern quill::Logger* global_logger_a;

int main()
{
  setup_quill("recommended_usage.log");

  // Each logger has its own runtime log level. With multiple loggers you can give different
  // subsystems different levels while still using the same wrapper-library pattern.
  global_logger_a->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(global_logger_a, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(global_logger_a, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(global_logger_a, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(global_logger_a, "This is a log debug example {}", 4);
  LOG_INFO(global_logger_a, "This is a log info example {}", 5);
  LOG_WARNING(global_logger_a, "This is a log warning example {}", 6);
  LOG_ERROR(global_logger_a, "This is a log error example {}", 7);
  LOG_CRITICAL(global_logger_a, "This is a log critical example {}", 118);
}
