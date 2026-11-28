#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstddef>
#include <thread>
#include <utility>

// Configure large initial capacity to avoid hot path allocations
struct MyFrontendOptions : quill::FrontendOptions
{
  static constexpr size_t initial_queue_capacity = 64 * 1024 * 1024; // 64MB
  // Other FrontendOptions values are inherited unchanged
};

// Every call of the application uses the same FrontendOptions
using MyFrontend = quill::FrontendImpl<MyFrontendOptions>;
using MyLogger = quill::LoggerImpl<MyFrontendOptions>;

int main()
{
  quill::Backend::start();

  auto console_sink = MyFrontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  MyLogger* logger = MyFrontend::create_or_get_logger("root", std::move(console_sink));

  // Hot path threads: Use the large queue (no shrinking needed)
  LOG_INFO(logger, "High frequency logging...");

  // Cold path threads: Shrink to smaller size after initialization
  std::thread cold_path_thread{[logger]()
                               {
                                 MyFrontend::shrink_thread_local_queue(128 * 1024); // Shrink to 128KB
                                 LOG_INFO(logger, "Infrequent logging...");
                               }};

  cold_path_thread.join();
}
