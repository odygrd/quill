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
 * Quill is header-only. When multiple shared libraries (DSOs) include Quill headers, each DSO
 * gets its own copy of all inline code and static data unless the linker deduplicates them.
 * The recommended pattern is the "single owner" approach demonstrated here: exactly one shared
 * library includes Quill headers and owns the backend, singletons, and loggers. All other
 * DSOs interact with Quill only through exported wrapper functions (setup_quill(), get_logger(), etc.).
 *
 * This pattern works correctly on all platforms (Linux, macOS, Windows) without requiring
 * special linker flags.
 *
 * === Platform-Specific Notes ===
 *
 * --- Linux ---
 * - Do NOT use -Bsymbolic or -Bsymbolic-functions linker flags with Quill's shared library.
 *   These flags prevent the dynamic linker from merging inline symbols across DSOs, causing
 *   Quill's singletons (LoggerManager, BackendManager, etc.) to be duplicated — which breaks
 *   the library (invisible loggers, multiple backend threads, lost log messages).
 * - When using dlopen/dlclose: you MUST flush all loggers and call Backend::stop() before
 *   calling dlclose() on any DSO that used Quill. Otherwise, the backend thread may
 *   dereference pointers into unmapped memory (MacroMetadata, decode functions, Sink vtables)
 *   causing SIGSEGV. See the flush_before_dlclose() example pattern below.
 *
 * --- macOS ---
 * - macOS uses two-level namespaces by default (since macOS 10.2). This means each dylib
 *   resolves symbols to its own copy, even if they have default visibility. Quill's
 *   QUILL_EXPORT (visibility("default")) alone is NOT sufficient to deduplicate singletons
 *   across dylibs.
 * - The "single owner" pattern shown here is the recommended solution on macOS.
 * - Alternatively, you can pass -flat_namespace to the linker, but this has global side
 *   effects and is generally not recommended.
 * - The same dlclose guidance as Linux applies (use dlclose(handle, RTLD_NODELETE) or
 *   flush + stop before unloading).
 *
 * --- Windows ---
 * - Set QUILL_DLL_EXPORT when building the shared library and QUILL_DLL_IMPORT when
 *   consuming it. CMake's WINDOWS_EXPORT_ALL_SYMBOLS is an optional CMake auto-export
 *   mechanism for projects without explicit dllexport annotations; it is not required by
 *   this example and has no direct preprocessor equivalent in MSBuild.
 * - When using LoadLibrary/FreeLibrary: call flush_log() in DllMain during
 *   DLL_PROCESS_DETACH before the DLL is unloaded:
 *
 *   BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
 *   {
 *     switch (ul_reason_for_call)
 *     {
 *     case DLL_PROCESS_ATTACH:
 *       break;
 *     case DLL_THREAD_ATTACH:
 *     case DLL_THREAD_DETACH:
 *       break;
 *     case DLL_PROCESS_DETACH:
 *       global_logger_a->flush_log();
 *       break;
 *     }
 *     return TRUE;
 *   }
 *
 * === dlopen/dlclose Usage (Linux/macOS) ===
 *
 * If you dynamically load a shared library that uses Quill via dlopen(), you must ensure
 * all log messages are flushed before calling dlclose():
 *
 *   void* handle = dlopen("libmy_plugin.so", RTLD_NOW);
 *   // ... use the plugin, which logs via Quill ...
 *
 *   // Option A: Flush and stop before unloading
 *   auto flush_fn = (void(*)()) dlsym(handle, "flush_quill_logs");
 *   if (flush_fn) flush_fn();  // plugin exports a flush wrapper
 *   dlclose(handle);
 *
 *   // Option B: Use RTLD_NODELETE to prevent actual unloading (keeps DSO mapped)
 *   // dlclose(handle);  // with RTLD_NODELETE flag at dlopen time
 *
 * Inside the plugin shared library, you can use __attribute__((destructor)) on Linux/macOS
 * as a safety net:
 *
 *   __attribute__((destructor)) void on_unload()
 *   {
 *     if (global_logger) global_logger->flush_log();
 *   }
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
