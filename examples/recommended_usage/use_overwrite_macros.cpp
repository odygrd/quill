#include "quill_wrapper/overwrite_macros.h"
#include "quill_wrapper/quill_wrapper.h"

int main()
{
  setup_quill("use_overwrite_macros.log");

  // Change the LogLevel to print everything
  global_logger_a->set_log_level(quill::LogLevel::TraceL3);

  LOG_INFO("This is a log info example {}", 5);
  LOG_WARNING("This is a log warning example {}", 6);
}