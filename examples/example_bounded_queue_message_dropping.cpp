// ALWAYS define QUILL_USE_BOUNDED_QUEUE before including quill.
// A better option is to ALWAYS pass this flag when you are building e.g in CMake target_compile_definitions(<target> PUBLIC QUILL_USE_BOUNDED_QUEUE)
#define QUILL_USE_BOUNDED_QUEUE

#include "quill/Quill.h"

int main()
{
  std::shared_ptr<quill::Handler> handler = quill::stdout_handler(); /** for stdout **/
  // std::shared_ptr<quill::Handler> handler = quill::file_handler("quickstart.log", "w");  /** for writing to file **/
  handler->set_pattern("%(ascii_time) [%(thread)] %(fileline:<28) LOG_%(level_name) %(message)");

  // set configuration
  quill::Config cfg;
  cfg.default_handlers.push_back(handler);

  // to simulate message dropping easily we can to set a high sleep duration to the logging thread
  cfg.backend_thread_sleep_duration = std::chrono::seconds{1};

  // and a small queue size
  cfg.default_queue_capacity = 4'096;

  // Apply configuration and start the backend worker thread
  quill::configure(cfg);
  quill::start();

  // In the following example you will see e.g.
  // 10:53:03 Quill INFO: dropped 7855 log messages from thread 7104
  for (size_t i = 0; i < 4000; ++i)
  {
    LOG_INFO(quill::get_logger(), "Hello {} #{}", "world", i);
    LOG_ERROR(quill::get_logger(), "This is a log error example {} #{}", 7, i);
  }
}