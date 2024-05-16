#include "quill/Quill.h"
#include "quill/handlers/JsonConsoleHandler.h"
#include <regex>

/**
 * Structured logging example
 */

int main()
{
  quill::Config cfg;

  // use the json handler
  std::shared_ptr<quill::Handler> json_handler = quill::json_console_handler();

  // Change how the date is formatted in the structured log.
  // JsonFileHandler must always have an empty pattern "" as the first argument.
  json_handler->set_pattern("", std::string{"%Y-%m-%d %H:%M:%S.%Qus"});

  // set this handler as the default for any new logger we are creating
  cfg.default_handlers.emplace_back(json_handler);

  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  quill::Logger* logger = quill::create_logger("json_logger");
  for (int i = 0; i < 2; ++i)
  {
    LOG_INFO(logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }

  for (int i = 0; i < 2; ++i)
  {
    LOG_WARNING(logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }
}
