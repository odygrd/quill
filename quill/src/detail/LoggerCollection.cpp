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
extern Logger* _g_root_logger;

namespace detail
{
/***/
LoggerCollection::LoggerCollection(Config const& config, ThreadContextCollection& thread_context_collection,
                                   HandlerCollection& handler_collection)
  : _config(config), _thread_context_collection(thread_context_collection), _handler_collection(handler_collection)
{
  // Pre-allocate early to a reasonable size
  _logger_name_map.reserve(16);
  create_root_logger();
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
    return _root_logger;
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
  // Get a copy of the root logger handlers
  std::vector<std::shared_ptr<Handler>> handlers = _root_logger->_logger_details.handlers();

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
Logger* LoggerCollection::create_logger(std::string const& logger_name, std::shared_ptr<Handler> handler,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock)
{
  // Register the handler, even if it already exists
  _handler_collection.subscribe_handler(handler);

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, std::move(handler), timestamp_clock_type,
                                            timestamp_clock, _thread_context_collection)};

  std::lock_guard<std::recursive_mutex> const lock{_rmutex};

  auto const insert_result = _logger_name_map.emplace(std::string{logger_name}, std::move(logger));

  // Return the inserted logger or the existing logger
  return (*insert_result.first).second.get();
}

/***/
Logger* LoggerCollection::create_logger(std::string const& logger_name,
                                        std::initializer_list<std::shared_ptr<Handler>> handlers,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock)
{
  return create_logger(logger_name, std::vector<std::shared_ptr<Handler>>{handlers},
                       timestamp_clock_type, timestamp_clock);
}

/***/
QUILL_NODISCARD Logger* LoggerCollection::create_logger(std::string const& logger_name,
                                                        std::vector<std::shared_ptr<Handler>> handlers,
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
void LoggerCollection::remove_logger(Logger* logger)
{
  logger->invalidate();
  _has_invalidated_loggers.store(true, std::memory_order_release);
}

/***/
void LoggerCollection::enable_console_colours() noexcept
{
  // Get the previous created default stdout handler
  std::shared_ptr<Handler> stdout_stream_handler =
    _handler_collection.stdout_console_handler("stdout");
  assert(stdout_stream_handler && "stdout_stream_handler can not be nullptr");

  auto console_handler = reinterpret_cast<ConsoleHandler*>(stdout_stream_handler.get());
  console_handler->enable_console_colours();
}

/***/
Logger* LoggerCollection::root_logger() const noexcept { return _root_logger; }

/***/
void LoggerCollection::create_root_logger()
{
  std::lock_guard<std::recursive_mutex> const lock{_rmutex};

  if (!_root_logger)
  {
    // initial root logger creation once
    if (_config.default_handlers.empty())
    {
      // Add the default console handler to the root logger
      std::shared_ptr<Handler> stdout_stream_handler =
        _handler_collection.stdout_console_handler("stdout");

      if (_config.enable_console_colours)
      {
        static_cast<ConsoleHandler*>(stdout_stream_handler.get())->enable_console_colours();
      }

      _root_logger = create_logger(_config.default_logger_name, stdout_stream_handler,
                                   _config.default_timestamp_clock_type, _config.default_custom_timestamp_clock);
    }
    else
    {
      _root_logger = create_logger(_config.default_logger_name, _config.default_handlers,
                                   _config.default_timestamp_clock_type, _config.default_custom_timestamp_clock);
    }

    _g_root_logger = _root_logger;
  }
  else
  {
    // this will update the root logger after it is created.
    // we do not invalidate the initial _root_logger pointer, instead we update the logger
    // if configure is called again, we do not want to invalidate the pointer to the root logger
    if (_config.default_handlers.empty())
    {
      // Add the default console handler to the root logger
      std::shared_ptr<Handler> stdout_stream_handler =
        _handler_collection.stdout_console_handler("stdout");

      if (_config.enable_console_colours)
      {
        static_cast<ConsoleHandler*>(stdout_stream_handler.get())->enable_console_colours();
      }

      // Register the handler, even if it already exists
      _handler_collection.subscribe_handler(stdout_stream_handler);

      // get the pointer to the existing logger
      auto search = _logger_name_map.find(_root_logger->_logger_details.name());
      assert(search != _logger_name_map.end() &&
             "we must always find the previous root logger in the map");

      // get the owning pointer to the logger
      std::unique_ptr<Logger> logger = std::move(search->second);

      // now we can erase the logger from the map in case the name is different
      _logger_name_map.erase(search);

      // update the root logger
      logger->_logger_details._name = _config.default_logger_name;
      logger->_logger_details._handlers.clear();
      logger->_logger_details._handlers.push_back(stdout_stream_handler);
      logger->_logger_details._timestamp_clock_type = _config.default_timestamp_clock_type;
      logger->_custom_timestamp_clock = _config.default_custom_timestamp_clock;

      // add back the logger to the map
      _logger_name_map.emplace(std::string{_config.default_logger_name}, std::move(logger));
    }
    else
    {
      // Register the handlers, even if they already exist
      for (auto& handler : _config.default_handlers)
      {
        _handler_collection.subscribe_handler(handler);
      }

      // get the pointer to the existing logger
      auto search = _logger_name_map.find(_root_logger->_logger_details.name());
      assert(search != _logger_name_map.end() &&
             "we must always find the previous root logger in the map");

      // get the owning pointer to the logger
      std::unique_ptr<Logger> logger = std::move(search->second);

      // now we can erase the logger from the map in case the name is different
      _logger_name_map.erase(search);

      // update the root logger
      logger->_logger_details._name = _config.default_logger_name;
      logger->_logger_details._handlers = _config.default_handlers;
      logger->_logger_details._timestamp_clock_type = _config.default_timestamp_clock_type;
      logger->_custom_timestamp_clock = _config.default_custom_timestamp_clock;

      // add back the logger to the map
      _logger_name_map.emplace(std::string{_config.default_logger_name}, std::move(logger));
    }
  }
}

/***/
bool LoggerCollection::remove_invalidated_loggers(std::function<bool(void)> const& check_queues_empty)
{
  bool has_invalidated_loggers{false};

  if (_has_invalidated_loggers.load(std::memory_order_acquire))
  {
    has_invalidated_loggers = true;
    _has_invalidated_loggers.store(false, std::memory_order_release);
  }

  bool loggers_removed{false};
  if (has_invalidated_loggers)
  {
    std::lock_guard<std::recursive_mutex> const lock{_rmutex};
    for (auto it = std::begin(_logger_name_map); it != std::end(_logger_name_map);)
    {
      if (it->second->is_invalidated())
      {
        // check if the logger has any pending records in the queue
        if (!check_queues_empty())
        {
          // we have pending records in the queue, we can not remove the logger yet
          ++it;
          _has_invalidated_loggers.store(true, std::memory_order_release);
        }
        else
        {
          loggers_removed = true;
          it = _logger_name_map.erase(it);
        }
      }
      else
      {
        ++it;
      }
    }
  }

  return loggers_removed;
}
} // namespace detail
} // namespace quill