#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/RotatingFileSink.h"

#include <utility>

/**
 * This example demonstrates how to create a RotatingFileSink with daily rotation and automatic rotation based on maximum file size.
 * For additional configuration options, refer to RotatingFileSinkConfig.
 */

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto rotating_file_sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
    "rotating_file.log",
    []()
    {
      // See RotatingFileSinkConfig for more options
      quill::RotatingFileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
      cfg.set_rotation_time_daily("18:30");
      cfg.set_rotation_max_file_size(1024); // small value to demonstrate the example
      return cfg;
    }());

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(rotating_file_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                          "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  for (int i = 0; i < 20; ++i)
  {
    LOG_INFO(logger, "Hello from rotating logger, index is {}", i);
  }
}
