#include "quill_shared.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

// Optional convenience for the shared-library example. A real wrapper can instead expose
// logger getter functions or manage several loggers.
QUILL_EXPORT quill::Logger* global_logger_a;

void setup_quill()
{
  // Start the backend thread
  quill::Backend::start();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // Create and store the logger
  global_logger_a = quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                   "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   "%H:%M:%S.%Qns", quill::Timezone::GmtTime});
}

quill::Logger* get_logger(std::string const& name) { return quill::Frontend::get_logger(name); }

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_WIN32)
/**
 * Safety net for dlclose() on Linux/macOS: flush pending log messages before the DSO is unmapped.
 * This prevents the backend thread from dereferencing dangling pointers to MacroMetadata,
 * decode functions, or Sink vtables that lived in this DSO's code segment.
 *
 * Note: This does NOT make dlclose() fully safe with Quill. The recommended approach is to
 * call Backend::stop() before dlclose(), or use RTLD_NODELETE to keep the DSO mapped.
 * This destructor is a best-effort safeguard only.
 */
__attribute__((destructor)) void quill_shared_lib_on_unload()
{
  if (global_logger_a)
  {
    global_logger_a->flush_log();
  }
}
#endif
