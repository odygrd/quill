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
  void write_log(quill::MacroMetadata const* /** log_metadata **/, uint64_t /** log_timestamp **/,
                 std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                 std::string const& /** process_id **/, std::string_view /** logger_name **/,
                 quill::LogLevel /** log_level **/, std::string_view /** log_level_description **/,
                 std::string_view /** log_level_short_code **/,
                 std::vector<std::pair<std::string, std::string>> const* /** named_args - only populated when named args in the format placeholder are used **/,
                 std::string_view /** log_message **/, std::string_view log_statement) override
  {
    // This function is called by the logger backend worker thread for each LOG_* macro.

    // Typically, this is where you would write the message to a file, send it over the network, etc.

    // In this example, instead of immediately writing the log statement, we cache it.
    // This can be useful for batching log messages to a database for example.

    // The last character of log_statement is '\n', which we exclude by using size() - 1.
    _cached_log_statements.push_back(std::string{log_statement.data(), log_statement.size() - 1});
  }

  /***/
  void flush_sink() noexcept override
  {
    // This function is not called for each LOG_* invocation like the write function.

    // Instead, it is called periodically, when there are no more LOG_* writes left to process,
    // or when logger->flush() is invoked.

    // In this example, we output all our cached log statements at this point.

    for (auto const& message : _cached_log_statements)
    {
      std::cout << message << std::endl;
    }

    _cached_log_statements.clear();
  }

  /***/
  void run_periodic_tasks() noexcept override
  {
    // Executes periodic user-defined tasks. This function is frequently invoked by the backend thread's main loop.
    // Avoid including heavy tasks here to prevent slowing down the backend thread.

    // For example, this could be another place to submit a batch commit to a database, as this
    // function is called more frequently than `flush_sink`.
  }

private:
  std::vector<std::string> _cached_log_statements;
};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  auto file_sink = quill::Frontend::create_or_get_sink<UserSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

  LOG_INFO(logger, "Hello from {}", "sink example");
  LOG_INFO(logger, "Invoking user sink flush");

  logger->flush_log();

  LOG_INFO(logger, "Log more {}", 123);
}