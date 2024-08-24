#include "quill_wrapper.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

// Define a global variable for a logger to avoid looking up the logger each time.
// Additional global variables can be defined for additional loggers if needed.
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