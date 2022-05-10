#include "quill/Quill.h"
#include "quill/handlers/Handler.h"   // for Handler

#include <iostream>

class CustomHandler : public quill::Handler
{
public:
  CustomHandler() = default;

  ~CustomHandler() override = default;

  /**
   * Write a formatted message to the stream
   * @param formatted_log_message input message to write
   * @param log_message_timestamp log timestamp
   */
  void write(fmt::memory_buffer const& formatted_log_message,
             std::chrono::nanoseconds log_message_timestamp, quill::LogLevel log_message_severity) override
  {
    // write a formatted log
    std::string log{formatted_log_message.data(), formatted_log_message.size()};
    std::cout << log << "\n";
  }

  /**
   * Flushes the stream
   */
  void flush() noexcept override
  {
    // flush
  }
};

int main()
{
  // Start the logging backend thread
  quill::start();

  // Because foo already created the handler we will get a pointer to the existing handler
  quill::Handler* custom_handler = quill::create_handler<CustomHandler>("MyHandler");

  // Create a logger using this handler
  quill::Logger* logger_bar = quill::create_logger("logger_bar", custom_handler);

  LOG_INFO(logger_bar, "Hello from {}", "library bar");
}