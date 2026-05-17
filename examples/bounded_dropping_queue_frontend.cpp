#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <utility>

/**
 * This example demonstrates how to change the type of the Single Producer Single Consumer (SPSC)
 * queue used by the frontend.
 *
 * By default, the library uses an UnboundedBlocking queue, which starts small with
 * `initial_queue_capacity` and grows as needed.
 *
 * This example switches to a bounded dropping queue so the frontend never blocks and excess
 * log messages are dropped once the queue is full.
 */

/**
 * Create custom frontend options.
 */
struct CustomFrontendOptions : quill::FrontendOptions
{
  // Set the queue to BoundedDropping
  static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;

  // Set small capacity to demonstrate dropping messages in this example
  static constexpr size_t initial_queue_capacity = 256;
};

/**
 * Define matching Frontend and Logger types that use the custom frontend options.
 */
using CustomFrontend = quill::FrontendImpl<CustomFrontendOptions>;
using CustomLogger = quill::LoggerImpl<CustomFrontendOptions>;

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = CustomFrontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  CustomLogger* logger = CustomFrontend::create_or_get_logger("root", std::move(console_sink));

  for (int i = 0; i < 32; ++i)
  {
    LOG_INFO(logger, "Bounded queue example log message num {}", i);
  }
}
