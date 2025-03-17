/**
 * This example demonstrates the recommended setup for the Quill library.
 *
 * It is advisable to encapsulate the header-only library into a static library, which
 * you build once and link to your main  application.
 * This library should include `quill/backend` in the .cpp files.
 *
 * In your application, include only the following headers for logging:
 *
 * - For logger lookup or creation:
 *   #include "quill/Frontend.h"
 *
 * - For sink creation:
 *   #include "quill/sinks/.."
 *
 * - For logging:
 *   #include "quill/Logger.h"
 *   #include "quill/LogMacros.h"
 */

// Include our wrapper lib
#include "quill_static.h"

// We need only those two headers in order to log
#include "quill/LogMacros.h"
#include "quill/Logger.h"

// include the relevant header from `std` folder for serialising STL types performing the formatting
// on the backend logging thread see
// https://quillcpp.readthedocs.io/en/latest/cheat_sheet.html#logging-stl-library-types for example:
#include "quill/std/Array.h"
#include "quill/std/Pair.h"

#if defined(_WIN32)
  // also required for wide string logging on windows
  #include "quill/std/WideString.h"
#endif

// We utilize the global_logger_a from the quill_wrapper library.
// The use of a global logger is optional.
// Alternatively, we could include "quill/Frontend.h" and use `quill::Frontend::get_logger(..)`
// to obtain the created logger, or we could store it as a class member.
extern quill::Logger* global_logger_a;

int main()
{
  setup_quill("std_types_logging.log");

  // Change the LogLevel to print everything
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
