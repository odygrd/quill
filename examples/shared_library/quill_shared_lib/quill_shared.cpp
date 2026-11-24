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

void stop_quill()
{
  // Backend::stop() keeps the loggers and the sinks registered for the next start. The ones
  // that a library created are removed while the backend still runs, so that none of them is
  // left with the code of a library that is unloaded
  quill::Frontend::remove_logger_blocking(global_logger_a);
  global_logger_a = nullptr;

  quill::Backend::stop();
}
