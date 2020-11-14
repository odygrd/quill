#include "quill/Quill.h"
#include "quill/detail/misc/Common.h" // for filename_t
#include "quill/handlers/Handler.h"   // for Handler

#include <iostream>

class CustomHandler : public quill::Handler
{
public:
  CustomHandler() = default;

  ~CustomHandler() override = default;

  /**
   * Write a formatted log record to the stream
   * @param formatted_log_record input log record to write
   * @param log_record_timestamp log record timestamp
   */
  void write(fmt::memory_buffer const& formatted_log_record,
             std::chrono::nanoseconds log_record_timestamp, quill::LogLevel log_message_severity) override
  {
    // write a formatted log
    std::string log{formatted_log_record.data(), formatted_log_record.size()};
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