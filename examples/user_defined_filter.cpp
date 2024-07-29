#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/filters/Filter.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

/**
 * This example demonstrates the creation usage of a user defined filter.
 * When a filter is applied, log messages are still enqueued from the frontend to the backend.
 * Subsequently, the backend dynamically filters them based on a given condition.
 */

class UserFilter : public quill::Filter
{
public:
  UserFilter() : quill::Filter("filter_1"){};

  bool filter(quill::MacroMetadata const* /** log_metadata **/, uint64_t /** log_timestamp **/,
              std::string_view /** thread_id **/, std::string_view /** thread_name **/,
              std::string_view /** logger_name **/, quill::LogLevel log_level,
              std::string_view log_message, std::string_view /** log_statement **/) noexcept override
  {
    // for example filter out duplicate log files
    bool is_different = true;

    if ((last_log_level == log_level) && (log_message == last_message))
    {
      is_different = false;
    }

    last_message = log_message;
    last_log_level = log_level;

    // return true to log the message, false otherwise
    return is_different;
  }

private:
  std::string last_message;
  quill::LogLevel last_log_level{quill::LogLevel::None};
};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // Add the filter - adding filters is thread safe and can be called anytime
  console_sink->add_filter(std::make_unique<UserFilter>());

  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // Change the LogLevel to send everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_INFO(logger, "This is a log info example {}", 123);
  LOG_INFO(logger, "This is a log info example {}", 123);
  LOG_INFO(logger, "This is a log info example {}", 123);
  LOG_INFO(logger, "This is a log info example {}", 123);
  LOG_INFO(logger, "This is a log info example {}", 456);
  LOG_INFO(logger, "This is a log info example {}", 123);
}
