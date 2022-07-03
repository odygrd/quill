#include "quill/detail/LoggerCollection.h"
#include "quill/QuillError.h"               // for QUILL_THROW, QuillError
#include "quill/detail/HandlerCollection.h" // for HandlerCollection
#include "quill/detail/LoggerDetails.h"     // for LoggerDetails
#include "quill/detail/misc/Common.h"       // for QUILL_UNLIKELY
#include "quill/handlers/StreamHandler.h"   // for StreamHandler
#include <mutex>                            // for lock_guard
#include <utility>                          // for move, pair
#include <vector>                           // for vector

namespace quill
{
namespace detail
{
/***/
LoggerCollection::LoggerCollection(ThreadContextCollection& thread_context_collection,
                                   HandlerCollection& handler_collection, TimestampClockType timestamp_clock_type,
                                   TimestampClock* custom_timestamp_clock, std::string const& default_logger_name)
  : _thread_context_collection(thread_context_collection), _handler_collection(handler_collection)
{
  // Pre-allocate early to a reasonable size
  _logger_name_map.reserve(16);

  // Add the default console handler to the default logger
  Handler* stdout_stream_handler = _handler_collection.stdout_console_handler("stdout");

  _default_logger = create_logger(default_logger_name, stdout_stream_handler, timestamp_clock_type,
                                  custom_timestamp_clock);
}

/***/
Logger* LoggerCollection::get_logger(char const* logger_name /* = nullptr */) const
{
  if (logger_name)
  {
    std::string logger_name_str{logger_name};
    std::lock_guard<std::recursive_mutex> const lock{_rmutex};

    // Search for the logger
    auto const search = _logger_name_map.find(logger_name_str);
    if (QUILL_UNLIKELY(search == _logger_name_map.cend()))
    {
      // logger does not exist
      QUILL_THROW(QuillError{std::string{"logger does not exist. name: "} + logger_name_str});
    }

    return (*search).second.get();
  }
  else
  {
    return _default_logger;
  }
}

/***/
QUILL_NODISCARD std::unordered_map<std::string, Logger*> LoggerCollection::get_all_loggers() const
{
  std::unordered_map<std::string, Logger*> logger_names;

  std::lock_guard<std::recursive_mutex> const lock{_rmutex};
  logger_names.reserve(_logger_name_map.size());
  for (auto const& elem : _logger_name_map)
  {
    logger_names.emplace(elem.first, elem.second.get());
  }

  return logger_names;
}

/***/
Logger* LoggerCollection::create_logger(std::string const& logger_name, TimestampClockType timestamp_clock_type,
                                        TimestampClock* timestamp_clock)
{
  // Get a copy of the default logger handlers
  std::vector<Handler*> handlers = _default_logger->_logger_details.handlers();

  // Register the handlers, even if they already exist
  for (auto& handler : handlers)
  {
    _handler_collection.subscribe_handler(handler);
  }

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, handlers, timestamp_clock_type,
                                            timestamp_clock, _thread_context_collection)};

  std::lock_guard<std::recursive_mutex> const lock{_rmutex};

  // Place the logger in our map
  auto const insert_result = _logger_name_map.emplace(std::string{logger_name}, std::move(logger));

  // Return the inserted logger or the existing logger
  return (*insert_result.first).second.get();
}

/***/
Logger* LoggerCollection::create_logger(std::string const& logger_name, Handler* handler,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock)
{
  // Register the handler, even if it already exists
  _handler_collection.subscribe_handler(handler);

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, handler, timestamp_clock_type,
                                            timestamp_clock, _thread_context_collection)};

  std::lock_guard<std::recursive_mutex> const lock{_rmutex};

  auto const insert_result = _logger_name_map.emplace(std::string{logger_name}, std::move(logger));

  // Return the inserted logger or the existing logger
  return (*insert_result.first).second.get();
}

/***/
Logger* LoggerCollection::create_logger(std::string const& logger_name, std::initializer_list<Handler*> handlers,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock)
{
  return create_logger(logger_name, std::vector<Handler*>{handlers}, timestamp_clock_type, timestamp_clock);
}

/***/
QUILL_NODISCARD Logger* LoggerCollection::create_logger(std::string const& logger_name,
                                                        std::vector<Handler*> const& handlers,
                                                        TimestampClockType timestamp_clock_type,
                                                        TimestampClock* timestamp_clock)
{
  // Register the handlers, even if they already exist
  for (auto& handler : handlers)
  {
    _handler_collection.subscribe_handler(handler);
  }

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, handlers, timestamp_clock_type,
                                            timestamp_clock, _thread_context_collection)};

  std::lock_guard<std::recursive_mutex> const lock{_rmutex};

  auto const insert_result = _logger_name_map.emplace(std::string{logger_name}, std::move(logger));

  // Return the inserted logger or the existing logger
  return (*insert_result.first).second.get();
}

/***/
void LoggerCollection::erase_logger(std::string const& logger_name)
{
  std::lock_guard<std::recursive_mutex> const lock{_rmutex};
  _logger_name_map.erase(logger_name);
}

/***/
void LoggerCollection::enable_console_colours() noexcept
{
  // Get the previous created default stdout handler
  Handler* stdout_stream_handler = _handler_collection.stdout_console_handler("stdout");
  assert(stdout_stream_handler && "stdout_stream_handler can not be nullptr");

  auto console_handler = reinterpret_cast<ConsoleHandler*>(stdout_stream_handler);
  console_handler->enable_console_colours();
}

/***/
void LoggerCollection::set_default_logger(Logger* logger) noexcept { _default_logger = logger; }

/***/
Logger* LoggerCollection::default_logger() const noexcept { return _default_logger; }

} // namespace detail
} // namespace quill