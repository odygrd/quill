#include "quill/SimpleSetup.h"
#include "quill/LogFunctions.h"

int main()
{
  // log to the console
  auto* logger = quill::simple_logger();
  quill::info(logger, "Hello from {}!", "Quill");

  // log to a file
  auto* logger2 = quill::simple_logger("test.log");
  quill::warning(logger2, "This message goes to a file");
}