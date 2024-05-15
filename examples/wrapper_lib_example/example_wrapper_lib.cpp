// Include our wrapper lib
#include "quill_wrapper/quill_wrapper.h"

// We need only those two headers in order to log
#include "quill/Logger.h"
#include "quill/detail/LogMacros.h"

extern quill::Logger* global_logger_a;

int main()
{
  setup_quill("wrapper_lib.log");

  // Change the LogLevel to print everything
  global_logger_a->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(global_logger_a, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(global_logger_a, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(global_logger_a, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(global_logger_a, "This is a log debug example {}", 4);
  LOG_INFO(global_logger_a, "This is a log info example {}", 5);
  LOG_WARNING(global_logger_a, "This is a log warning example {}", 6);
  LOG_ERROR(global_logger_a, "This is a log error example {}", 7);
  LOG_CRITICAL(global_logger_a, "This is a log critical example {}", 11118);
}
