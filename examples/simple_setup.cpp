#include "quill/LogFunctions.h"
#include "quill/SimpleSetup.h"

/**
 * This example shows the smallest setup path using `SimpleSetup.h`.
 *
 * It creates ready-to-use loggers without explicitly calling `Backend::start()`
 * or creating sinks through the `Frontend` API.
 */

int main()
{
  auto* console_logger = quill::simple_logger();
  quill::info(console_logger, "Hello from {}!", "Quill");

  auto* file_logger = quill::simple_logger("simple_setup.log");
  quill::warning(file_logger, "This message goes to a file");
}
