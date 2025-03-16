#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <string>
#include <utility>

/**
 * The example demonstrates how to customise the console colours
 */

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Create the sink
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
    "sink_id_1",
    []()
    {
      quill::ConsoleSinkConfig config;

      quill::ConsoleSinkConfig::Colours colours;
      colours.assign_colour_to_log_level(quill::LogLevel::Info, quill::ConsoleSinkConfig::Colours::blue); // overwrite the colour for INFO

      config.set_colours(colours);
      return config;
    }());

  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
  LOG_INFO(logger, "This is a log info example {}", sizeof(std::string));
  LOG_WARNING(logger, "This is a log warning example {}", sizeof(std::string));
  LOG_ERROR(logger, "This is a log error example {}", sizeof(std::string));
  LOG_CRITICAL(logger, "This is a log critical example {}", sizeof(std::string));
}
