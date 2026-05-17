#include "quill/LogMacros.h"
#include "quill/SimpleSetup.h"

int main()
{
  // log to the console
  auto* logger = quill::simple_logger();
  LOG_INFO(logger, "Hello from {}!", "Quill");

  // log to a file
  auto* logger2 = quill::simple_logger("test.log");
  LOG_WARNING(logger2, "This message goes to a file");

  // If you also want Quill's built-in signal handler, use:
  // auto* logger3 = quill::simple_logger_with_signal_handler();
}
