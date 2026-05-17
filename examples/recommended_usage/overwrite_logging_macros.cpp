#include "overwrite_macros.h"
#include "quill_static.h"

/**
 * This example shows how to use wrapper-provided logging macros that already
 * bind a preconfigured default logger from the recommended static-library setup.
 *
 * This is optional convenience only. If your project uses multiple loggers, keeping the
 * logger argument explicit is usually clearer.
 */

int main()
{
  setup_quill("overwrite_logging_macros.log");

  // Change the LogLevel to print everything
  global_logger_a->set_log_level(quill::LogLevel::TraceL3);

  LOG_INFO("This is a log info example {}", 5);
  LOG_WARNING("This is a log warning example {}", 6);
}
