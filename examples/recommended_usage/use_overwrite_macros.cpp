#include "quill_static_lib/overwrite_macros.h"
#include "quill_static_lib/quill_static.h"

int main()
{
  setup_quill("use_overwrite_macros.log");

  // Change the LogLevel to print everything
  global_logger_a->set_log_level(quill::LogLevel::TraceL3);

  LOG_INFO("This is a log info example {}", 5);
  LOG_WARNING("This is a log warning example {}", 6);
}