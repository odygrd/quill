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
 * - Before calling dlclose(), follow the shutdown sequence below for the loggers that the
 *   library uses. The backend can still hold pointers into the library until that sequence
 *   completes.
 *
 * --- macOS ---
 * - macOS uses two-level namespaces by default (since macOS 10.2). This means each dylib
 *   resolves symbols to its own copy, even if they have default visibility. Quill's
 *   QUILL_EXPORT (visibility("default")) alone is NOT sufficient to deduplicate singletons
 *   across dylibs.
 * - The "single owner" pattern shown here is the recommended solution on macOS.
 * - Alternatively, you can pass -flat_namespace to the linker, but this has global side
 *   effects and is generally not recommended.
 * - The same dlclose guidance as Linux applies.
 *
 * --- Windows ---
 * - Set QUILL_DLL_EXPORT when building the shared library and QUILL_DLL_IMPORT when
 *   consuming it. CMake's WINDOWS_EXPORT_ALL_SYMBOLS is an optional CMake auto-export
 *   mechanism for projects without explicit dllexport annotations; it is not required by
 *   this example and has no direct preprocessor equivalent in MSBuild.
 * - When using LoadLibrary/FreeLibrary: call a function that the DLL exports to stop the
 *   backend, such as stop_quill() of this example, before FreeLibrary(). Do not call
 *   flush_log() or Backend::stop() from DllMain during DLL_PROCESS_DETACH: the loader lock is
 *   held there, which the backend thread needs to exit, and when the process is exiting that
 *   thread was already terminated, so the call waits forever.
 *
 * === dlopen/dlclose Usage (Linux/macOS) ===
 *
 * If you dynamically load a shared library that uses Quill via dlopen(), shut its logging down
 * before calling dlclose(), with a function that the library exports with C linkage, such as
 * stop_quill() of this example. Unload the library only after that function has returned:
 *
 *   void* handle = dlopen("libmy_plugin.so", RTLD_NOW);
 *   // ... use the plugin, which logs via Quill ...
 *
 *   // Option A: Shut the logging of the plugin down, then unload it
 *   auto stop_fn = (void(*)()) dlsym(handle, "stop_quill");
 *   if (stop_fn)
 *   {
 *     stop_fn();
 *     dlclose(handle);
 *   }
 *
 *   // Option B: Use RTLD_NODELETE to prevent actual unloading (keeps DSO mapped)
 *   // dlclose(handle);  // with RTLD_NODELETE flag at dlopen time
 *
 * The function has to release the sink pointers that the library holds, then remove the
 * loggers that the library created, with Frontend::remove_logger_blocking() while the backend
 * still runs. If the library started the backend, it then stops it. Removing the logger
 * unregisters its sink once nothing holds the sink. A sink released later stays registered.
 * Backend::stop() keeps the loggers and the sinks registered for the next start, and those of
 * the library must not outlive its code. It then writes the log messages that are still queued.
 *
 * This is for a library that owns its loggers, as the one of this example does. A plugin that
 * logs through a logger of the host application has no logger to remove. The records that its
 * threads logged have to be written before the plugin is unloaded: a record refers to the log
 * statement that made it, which is in the plugin. Each thread of the plugin calls flush_log() on
 * the logger after its last log statement of the plugin, and then the thread that unloads the
 * plugin calls flush_backtrace(), for a logger that stores a backtrace, and flush_log().
 *
 * Do not flush or stop from a function of the library that runs when it is unloaded, such as
 * one declared with __attribute__((destructor)): when the process exits, it runs after the
 * objects that Quill keeps were destroyed.
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

  // Before the library is unloaded
  stop_quill();
}
