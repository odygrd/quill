/**
 * This example demonstrates the recommended setup for the Quill library.
 *
 * It is advisable to encapsulate the header-only library into a static library, which
 * you build once and link to your main  application.
 * This library should include `quill/backend` in the .cpp files.
 *
 * In your application, include only the following headers for logging:
 *
 * - For logger lookup or creation:
 *   #include "quill/Frontend.h"
 *
 * - For sink creation:
 *   #include "quill/sinks/.."
 *
 * - For logging:
 *   #include "quill/Logger.h"
 *   #include "quill/LogMacros.h"
 */

// Include our wrapper lib
#include "quill_wrapper/quill_wrapper.h"

// We need only those two headers in order to log
#include "quill/LogMacros.h"
#include "quill/Logger.h"

// We utilize the global_logger_a from the quill_wrapper library.
// The use of a global logger is optional.
// Alternatively, we could include "quill/Frontend.h" and use `quill::Frontend::get_logger(..)`
// to obtain the created logger, or we could store it as a class member.
extern quill::Logger* global_logger_a;

int main()
{
  setup_quill("recommended_usage.log");

  // Change the LogLevel to print everything
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