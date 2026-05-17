#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

/**
 * This example demonstrates defining and utilizing custom FrontendOptions.
 * It's useful when you need to modify the queue type or capacity.
 * FrontendOptions are compile-time options and must be passed as a template argument.
 */

// Derive from FrontendOptions and override only the values that differ.
struct CustomFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
};

// To utilize our custom FrontendOptions, we define a Frontend class using CustomFrontendOptions
using CustomFrontend = quill::FrontendImpl<CustomFrontendOptions>;

// The Logger type must also be defined
using CustomLogger = quill::LoggerImpl<CustomFrontendOptions>;

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options); // or quill::Backend::start<CustomFrontendOptions>(backend_options, signal_handler_options);

  // All frontend operations must utilize CustomFrontend instead of quill::Frontend
  auto console_sink = CustomFrontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  CustomLogger* logger = CustomFrontend::create_or_get_logger("root", std::move(console_sink));

  // log something
  LOG_INFO(logger, "This is a log info example {}", 123);
  LOG_WARNING(logger, "This is a log warning example {}", 123);
}
