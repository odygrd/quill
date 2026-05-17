#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <utility>

int main()
{
  quill::Backend::start();

  // Frontend
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    "trivial_logging.log",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

  LOG_INFO(logger, "log something {}", 123);
  LOG_WARNING(logger, "something else {}", 456);
}