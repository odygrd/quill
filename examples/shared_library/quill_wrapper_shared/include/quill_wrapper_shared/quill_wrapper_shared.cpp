#include "quill_wrapper_shared.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"

// Define a global variable for a logger to avoid looking up the logger each time.
// Additional global variables can be defined for additional loggers if needed.
QUILL_EXPORT quill::Logger* global_logger_a;

void setup_quill()
{
  // Start the backend thread
  quill::Backend::start();

    // log everything to console
    auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
    "console_sink");

  // log everything to file
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    "debug_logs.txt", []() {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(
        quill::FilenameAppendOption::StartDateTime);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  // create logger
  global_logger_a = quill::Frontend::create_or_get_logger(
    "root", {std::move(console_sink), std::move(file_sink)});
}