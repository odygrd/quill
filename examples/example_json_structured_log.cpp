#include "quill/Quill.h"
#include "quill/handlers/JsonFileHandler.h"
#include <regex>

/**
 * Trivial logging example
 */

int main()
{
  quill::Config cfg;

  // use the json handler
  std::shared_ptr<quill::Handler> json_handler =
    quill::json_file_handler("json_output.log",
                             []()
                             {
                               quill::JsonFileHandlerConfig cfg;
                               cfg.set_open_mode('w');
                               cfg.set_append_to_filename(quill::FilenameAppend::StartDateTime);
                               return cfg;
                             }());

  // Change how the date is formatted in the structured log.
  // JsonFileHandler must always have an empty pattern "" as the first argument.
  json_handler->set_pattern("", std::string{"%Y-%m-%d %H:%M:%S.%Qus"});

  // set this handler as the default for any new logger we are creating
  cfg.default_handlers.emplace_back(json_handler);

  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  // log to the json file ONLY by using the default logger
  quill::Logger* logger = quill::get_logger();
  for (int i = 0; i < 2; ++i)
  {
    LOG_INFO(logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }

  // or create another logger tha logs e.g. to stdout and to the json file at the same time
  quill::Logger* dual_logger = quill::create_logger("dual_logger", {quill::stdout_handler(), json_handler});
  for (int i = 2; i < 4; ++i)
  {
    LOG_INFO(dual_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }
}
