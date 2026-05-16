#include "quill/LogFunctions.h"
#include "quill/SimpleSetup.h"

int main()
{
  // log to the console
  auto* logger = quill::simple_logger();
  quill::info(logger, "Hello from {}!", "Quill");

  // log to a file
  auto* logger2 = quill::simple_logger("test.log");
  quill::warning(logger2, "This message goes to a file");

  // If you also want Quill's built-in signal handler, use:
  // auto* logger3 = quill::simple_logger_with_signal_handler();
}