/**
 * This example demonstrates the recommended Quill integration style for larger projects.
 *
 * Build your Quill setup code into your own small static library once, then link that library
 * into the rest of the application. Keep `Backend`, `Frontend`, sink creation, and logger
 * creation inside that wrapper library.
 *
 * This example exposes one global logger purely as a simple convenience. That is not a Quill
 * requirement. Your wrapper can instead expose logger getter functions, return logger pointers
 * from setup code, store loggers in objects, or manage multiple named loggers.
 *
 * Multiple loggers are often a better fit for larger applications because they allow different
 * subsystems to use different names, sinks, and log levels.
 *
 * In application code that only logs, include only the lightweight headers you need:
 *
 * - For logging:
 *   #include "quill/Logger.h"
 *   #include "quill/LogMacros.h"
 *
 * - For logger lookup or creation when needed:
 *   #include "quill/Frontend.h"
 *
 * - For sink creation when needed:
 *   #include "quill/sinks/.."
 */

// Include our wrapper lib
#include "quill_static.h"

// We need only those two headers in order to log
#include "quill/LogMacros.h"
#include "quill/Logger.h"

// include the relevant header from `std` folder for serialising STL types performing the formatting
// on the backend logging thread see
// https://quillcpp.readthedocs.io/en/latest/recipes.html#logging-stl-library-types for example:
#include "quill/std/Array.h"
#include "quill/std/Pair.h"

#if defined(_WIN32)
  // also required for wide string logging on windows
  #include "quill/std/WideString.h"
#endif

// This example uses a global logger exported by the wrapper library only to keep the example
// small. In real projects you can instead look up loggers with Frontend, expose getter
// functions from the wrapper library, or store logger pointers as members.
extern quill::Logger* global_logger_a;

int main()
{
  setup_quill("std_types_logging.log");

  // Each logger has its own runtime log level. With multiple loggers you can give different
  // subsystems different levels while still using the same wrapper-library pattern.
  global_logger_a->set_log_level(quill::LogLevel::TraceL3);
  std::array<std::string, 2> array = {"test", "string"};
  LOG_INFO(global_logger_a, "log an array {}", array);

  std::pair<std::string, uint32_t> pair = {"data", 1};
  LOG_INFO(global_logger_a, "log a pair {}", pair);

#if defined(_WIN32)
  std::array<std::wstring, 2> wide_array = {L"wide_test", L"wide_string"};
  LOG_INFO(global_logger_a, "log wide array {}", wide_array);

  std::pair<std::wstring, uint32_t> wide_pair = {L"wide_data", 1};
  LOG_INFO(global_logger_a, "log a wide pair {}", wide_pair);
#endif
}
