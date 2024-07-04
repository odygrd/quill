#include "quill_wrapper_shared/quill_wrapper_shared.h"

#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"

#include <iostream>
#include <string>
#include <string_view>

/**
 * @brief Example of building Quill as a shared library.
 *
 * To build Quill as a shared library on Windows, follow these steps:
 * - Ensure the `QUILL_DLL_EXPORT` and `QUILL_DLL_IMPORT` flags are set as required
 * - You also need to set the CMake option: `-DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE`
 */
QUILL_EXPORT extern quill::Logger* global_logger_a;

int main()
{
  setup_quill();

  // Change the LogLevel to print everything
  global_logger_a->set_log_level(quill::LogLevel::TraceL3);

  std::string s {"string"};
  std::string_view sv {"string_view"};

  LOG_TRACE_L3(global_logger_a, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(global_logger_a, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(global_logger_a, "This is a log trace l1 {} example", "s");
  LOG_DEBUG(global_logger_a, "This is a log debug example {}", 4);
  LOG_INFO(global_logger_a, "This is a log info example {}", s);
  LOG_WARNING(global_logger_a, "This is a log warning example {}", sv);
  LOG_ERROR(global_logger_a, "This is a log error example {}", 7);
  LOG_CRITICAL(global_logger_a, "This is a log critical example {}", 118);

  global_logger_a->flush_log();
}