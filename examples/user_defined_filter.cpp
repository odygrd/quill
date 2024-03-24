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

  bool filter(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp,
              std::string_view thread_id, std::string_view thread_name, std::string_view logger_name,
              quill::LogLevel log_level, std::string_view log_message) noexcept override
  {
    // for example filter out lines 47 and 48 of any file
    return (std::stoi(log_metadata->line()) != 47) && (std::stoi(log_metadata->line()) != 48);
  }
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

  LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
  LOG_DEBUG(logger, "This is a log debug example {}", 4);
  LOG_INFO(logger, "This is a log info example {}", sizeof(std::string));
  LOG_WARNING(logger, "This is a log warning example {}", sizeof(std::string));
  LOG_ERROR(logger, "This is a log error example {}", sizeof(std::string));
  LOG_CRITICAL(logger, "This is a log critical example {}", sizeof(std::string));
}
