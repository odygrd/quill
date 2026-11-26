#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

// Each logging thread gets a fixed 16 KiB queue that never grows. When it is full, log messages
// are dropped instead of blocking the thread
struct SmallFrontendOptions : quill::FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
  static constexpr size_t initial_queue_capacity = 16u * 1024u;
};

using SmallFrontend = quill::FrontendImpl<SmallFrontendOptions>;
using SmallLogger = SmallFrontend::logger_t;

int main()
{
  // The backend buffers fewer events per logging thread, so it stops reading a queue sooner
  quill::BackendOptions backend_options;
  backend_options.transit_event_buffer_initial_capacity = 16;
  backend_options.transit_events_soft_limit = 64;
  backend_options.transit_events_hard_limit = 256;
  quill::Backend::start(backend_options);

  auto sink = SmallFrontend::create_or_get_sink<quill::ConsoleSink>("small_footprint_console");
  SmallLogger* logger = SmallFrontend::create_or_get_logger("small_footprint_logger", sink);

  LOG_INFO(logger, "Logging with a bounded {} KiB queue", 16);
}
