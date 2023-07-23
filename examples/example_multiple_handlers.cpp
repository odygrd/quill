#include "quill/Quill.h"

#include <iostream>
#include <string>
#include <vector>

/**
 * A custom handler class that appends formatted logs to a vector
 */
class VectorHandler final : public quill::Handler
{
public:
  VectorHandler() = default;

  /***/
  void write(quill::fmt_buffer_t const& formatted_log_message, quill::TransitEvent const& log_event) override
  {
    // Called by the logger backend worker thread for each LOG_* macro
    // formatted_log_message.size() - 1 to exclude '/n'
    _formatted_messages.emplace_back(
      std::string{formatted_log_message.begin(), formatted_log_message.size() - 1});
  }

  /***/
  void flush() noexcept override
  {
    // Called by the logger backend worker thread
    // This is not called for each LOG_* invocation like the write function, instead it is called
    // periodically or when there are no more LOG_* writes left to process.
    std::cout << fmtquill::format("VectorHandler: {}", _formatted_messages) << std::endl;
  }

private:
  std::vector<std::string> _formatted_messages;
};

int main()
{
  // Start the logging backend thread
  quill::start();

  // Get a handler to the file
  std::shared_ptr<quill::Handler> file_handler = quill::file_handler("app.log",
                                                                     []()
                                                                     {
                                                                       quill::FileHandlerConfig cfg;
                                                                       cfg.set_open_mode('w');
                                                                       return cfg;
                                                                     }());

  // Get the handler to console
  std::shared_ptr<quill::Handler> console_handler = quill::stdout_handler();

  // Create a logger using this handler
  // create_handler(unique handler name, constructor args...)
  std::shared_ptr<quill::Handler> vector_handler =
    quill::create_handler<VectorHandler>("my_vector_handler");

  // Customise vector_handler format
  vector_handler->set_pattern("%(level_name) %(logger_name) - %(message)");

  quill::Logger* logger_foo =
    quill::create_logger("my_logger", {file_handler, console_handler, vector_handler});
  logger_foo->set_log_level(quill::LogLevel::Debug);

  LOG_INFO(logger_foo, "Hello from {}", "quill");
  LOG_DEBUG(logger_foo, "Multiple handlers {}", "example");
}