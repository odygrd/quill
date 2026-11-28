#include "quill/LogFunctions.h"
#include "quill/SimpleSetup.h"

int main()
{
  auto* logger = quill::simple_logger();
  quill::info(logger, "Hello from {}!", "Quill");
}
