#include "quill_shared.h"

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
 *
 * Note:
 * On Windows, when using Quill inside a DLL that is dynamically loaded with `LoadLibrary` and then
 * freed with `FreeLibrary` at runtime, it's important to call `flush_log()` in DllMain during
 * `DLL_PROCESS_DETACH` in each DLL that is being unloaded.
 *
 * This ensures that no pending logs remain in the DLL being freed.
 *
 * BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
 * {
 *   switch (ul_reason_for_call)
 *   {
 *   case DLL_PROCESS_ATTACH:
 *     // Code to run when the DLL is loaded
 *     break;
 *   case DLL_THREAD_ATTACH:
 *   case DLL_THREAD_DETACH:
 *     // Code to run when a thread is created or destroyed
 *     break;
 *   case DLL_PROCESS_DETACH:
 *     // Ensure any pending logs are flushed before the DLL is unloaded
 *     global_logger_a->flush_log();
 *     break;
 *   }
 *   return TRUE; // Successfully processed
 * }
 */

QUILL_EXPORT extern quill::Logger* global_logger_a;

int main()
{
  setup_quill();

  QUILL_ASSERT(global_logger_a == get_logger("root"), "global_logger_a is not set");

  // Change the LogLevel to print everything
  global_logger_a->set_log_level(quill::LogLevel::TraceL3);

  std::string s{"string"};
  std::string_view sv{"string_view"};

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
