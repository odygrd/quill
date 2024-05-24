#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/Sink.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/**
 * This example demonstrates how to implement a custom Sink
 */

class UserSink final : public quill::Sink
{
public:
  UserSink() = default;

  /***/
  void write_log_message(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp,
                         std::string_view thread_id, std::string_view thread_name,
                         std::string_view logger_name, quill::LogLevel log_level,
                         std::vector<std::pair<std::string, std::string>> const* named_args,
                         std::string_view log_message) override
  {
    // Called by the logger backend worker thread for each LOG_* macro
    // last character is '\n' and we exclude it using size() - 1
    _messages.push_back(std::string{log_message.data(), log_message.size() - 1});
  }

  /***/
  void flush_sink() noexcept override
  {
    // This is not called for each LOG_* invocation like the write function, instead it is called
    // periodically or when there are no more LOG_* writes left to process or when logger->flush()
    for (auto const& message : _messages)
    {
      std::cout << message << std::endl;
    }
    _messages.clear();
  }

  /***/
  void run_periodic_tasks() noexcept override
  {
    // Executes periodic user-defined tasks. This function is frequently invoked by the backend thread's main loop.
    // Avoid including heavy tasks here to prevent slowing down the backend thread.
  }

private:
  std::vector<std::string> _messages;
};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto file_sink = quill::Frontend::create_or_get_sink<UserSink>("sink_id_1");

  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

  logger->set_log_level(quill::LogLevel::Debug);

  LOG_INFO(logger, "Hello from {}", "sink example");
  LOG_DEBUG(logger, "Invoking user sink flush");

  logger->flush_log();

  LOG_INFO(logger, "Log more {}", 123);
}