#include "quill/Quill.h"
#include "quill/handlers/JsonFileHandler.h"
#include <regex>

std::pair<std::string, std::vector<std::string>> process_structured_log_template(std::string const& input)
{
  std::string fmt_str;
  std::vector<std::string> keys;

  size_t cur_pos = 0;

  size_t open_bracket_pos = input.find_first_of('{');
  while (open_bracket_pos != std::string::npos)
  {
    // found an open bracket
    size_t const open_bracket_2_pos = input.find_first_of('{', open_bracket_pos + 1);

    if (open_bracket_2_pos != std::string::npos)
    {
      // found another open bracket
      if ((open_bracket_2_pos - 1) == open_bracket_pos)
      {
        std::cout << "ignore " << std::endl;
        open_bracket_pos = input.find_first_of('{', open_bracket_2_pos + 1);
        continue;
      }
    }

    // look for the next close bracket
    size_t close_bracket_pos = input.find_first_of('}', open_bracket_pos + 1);
    while (close_bracket_pos != std::string::npos)
    {
      // found closed bracket
      size_t const close_bracket_2_pos = input.find_first_of('}', close_bracket_pos + 1);

      if (close_bracket_2_pos != std::string::npos)
      {
        // found another open bracket
        if ((close_bracket_2_pos - 1) == close_bracket_pos)
        {
          std::cout << "ignore cb " << std::endl;
          close_bracket_pos = input.find_first_of('}', close_bracket_2_pos + 1);
          continue;
        }
      }

      // construct a fmt string excluding the characters inside the brackets { }
      fmt_str += input.substr(cur_pos, open_bracket_pos - cur_pos) + "{}";
      cur_pos = close_bracket_pos + 1;

      // also add the keys to the vector
      keys.emplace_back(input.substr(open_bracket_pos + 1, (close_bracket_pos - open_bracket_pos - 1)));

      break;
    }

    open_bracket_pos = input.find_first_of('{', close_bracket_pos);
  }

  return std::make_pair(fmt_str, keys);
}

/**
 * Trivial logging example
 */

int main()
{
  quill::Config cfg;

  // use the json handler
  quill::Handler* json_handler =
    quill::create_handler<quill::JsonFileHandler>("json_output.log", "w", quill::FilenameAppend::DateTime);

  // set a custom data output
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
