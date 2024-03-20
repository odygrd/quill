#include "quill/Quill.h"
#include "quill/handlers/JsonFileHandler.h"
#include <regex>

/**
 * Structured logging example
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
  // This is only logged to the file
  quill::Logger* logger = quill::create_logger("json_only");
  for (int i = 0; i < 2; ++i)
  {
    LOG_INFO(logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }

  // or create another logger tha logs e.g. to stdout and to the json file at the same time
  // json is logged to the file
  // regural log is logged to stdout
  quill::Logger* flex_logger = quill::create_logger("both_formats", {quill::stdout_handler(), json_handler});
  for (int i = 2; i < 4; ++i)
  {
    LOG_INFO(flex_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
  }

  // same as the above with the inclusion of keys

  // create a custom stdout_handler to change the format
  auto custom_stdout_handler = quill::stdout_handler("custom_stdout");
  custom_stdout_handler->set_pattern(
    "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) "
    "%(logger:<12) %(message) [%(structured_keys)]");

  // Logged to both file as json and to stdout with the inclusion of keys for stdout
  quill::Logger* skey_logger =
    quill::create_logger("with_skey", {std::move(custom_stdout_handler), json_handler});
  for (int i = 2; i < 4; ++i)
  {
    LOG_INFO(skey_logger, "Method elapsed time [{method}, {endpoint}, {duration}]", "POST",
             "http://", 10 * i);
  }
}
