// When QUILL_ROOT_LOGGER_ONLY is defined then only a single root logger object is used
#define QUILL_ROOT_LOGGER_ONLY

#include "quill/Quill.h"

int main()
{
  std::shared_ptr<quill::Handler> handler = quill::stdout_handler(); /** for stdout **/
  // std::shared_ptr<quill::Handler> handler = quill::file_handler("quickstart.log", "w");  /** for writing to file **/
  handler->set_pattern("%(ascii_time) [%(thread)] %(fileline:<28) LOG_%(level_name) %(message)");

  // set configuration
  quill::Config cfg;
  cfg.default_handlers.push_back(handler);

  // Apply configuration and start the backend worker thread
  quill::configure(cfg);
  quill::start();

  LOG_INFO("Hello {}", "world");
  LOG_ERROR("This is a log error example {}", 7);
}