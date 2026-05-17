#include "quill_static.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

/**
 * Compile this file into your application's wrapper static library.
 * Keep Quill backend/frontend initialization and sink/logger creation here so the rest of
 * the application can log without pulling in the heavier setup headers everywhere.
 *
 * This example exports one global logger only to keep the example compact. In real projects
 * the wrapper library can expose getter functions or several loggers instead.
 */

// Optional convenience for the example. Larger applications often keep multiple loggers so
// different subsystems can use different names, sinks, and runtime log levels.
quill::Logger* global_logger_a;

void setup_quill(char const* log_file)
{
  // Start the backend thread
  quill::Backend::start();

  // Setup sink and logger
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    log_file,
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  // Create and store the logger
  global_logger_a = quill::Frontend::create_or_get_logger(
    "root", std::move(file_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                   "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   "%H:%M:%S.%Qns", quill::Timezone::GmtTime});
}
