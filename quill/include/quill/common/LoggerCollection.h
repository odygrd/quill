/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/clock/TimestampClock.h"
#include "quill/common/Attributes.h"
#include "quill/common/Common.h"
#include "quill/common/Config.h"
#include "quill/common/HandlerCollection.h"
#include "quill/common/Logger.h"
#include "quill/common/ThreadContextCollection.h"

#include <functional>
#include <initializer_list> // for initializer_list
#include <memory>           // for unique_ptr
#include <mutex>
#include <string>        // for string, hash
#include <unordered_map> // for unordered_map
#include <utility>
#include <vector>

namespace quill
{
namespace detail
{
/**
 * Contains all created loggers
 */
class LoggerCollection
{
public:
  /**
   * Constructor
   */
  LoggerCollection(Config const& config, ThreadContextCollection& thread_context_collection,
                   HandlerCollection& handler_collection)
    : _config(config), _thread_context_collection(thread_context_collection), _handler_collection(handler_collection)
  {
    // Pre-allocate early to a reasonable size
    _logger_name_map.reserve(16);
    create_root_logger();
  }

  /**
   * Destructor
   */
  ~LoggerCollection() = default;

  /**
   * Deleted
   */
  LoggerCollection(LoggerCollection const&) = delete;
  LoggerCollection& operator=(LoggerCollection const&) = delete;

  /**
   * Creates a new logger with default log level info or returns an existing logger with it's
   * cached log levels and handlers if the logger already exists
   * @param logger_name The name of the logger or empty for the root logger
   * @note this function is slow, consider calling it only once and store the pointer to the logger
   * @return a Logger object or the root logger is logger_name is empty
   */
  QUILL_NODISCARD Logger* get_logger(char const* logger_name = nullptr) const
  {
    if (logger_name)
    {
      std::string const logger_name_str{logger_name};
      std::lock_guard<std::recursive_mutex> const lock{_rmutex};

      // Search for the logger
      auto const search = _logger_name_map.find(logger_name_str);
      if (QUILL_UNLIKELY(search == _logger_name_map.cend()))
      {
        // logger does not exist
        QUILL_THROW(QuillError{std::string{"logger does not exist. name: "} + logger_name_str});
      }

      return search->second.get();
    }

    return _root_logger;
  }

  /**
   * Returns all existing loggers and the pointers to them
   * @return a map logger_name -> logger*
   */
  QUILL_NODISCARD std::unordered_map<std::string, Logger*> get_all_loggers() const
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

  /**
   * Create a new logger using the same handlers and formatter as the root logger
   * @param logger_name the name of the logger to add
   * @param timestamp_clock_type timestamp clock type
   * @param timestamp_clock timestamp clock
   * @return a pointer to the logger
   */
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, TimestampClockType timestamp_clock_type,
                                        TimestampClock* timestamp_clock)
  {
    // Get a copy of the root logger handlers
    std::vector<std::shared_ptr<Handler>> const handlers = _root_logger->_logger_details.handlers();

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
    return insert_result.first->second.get();
  }

  /**
   * Creates a new logger
   * @param logger_name the name of the logger to add
   * @param handler The handler for the logger
   * @param timestamp_clock_type timestamp clock type
   * @param timestamp_clock timestamp clock
   * @return a pointer to the logger
   */
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, std::shared_ptr<Handler> handler,
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
    return insert_result.first->second.get();
  }

  /**
   * Create a new logger with multiple handler
   * @param logger_name the name of the logger to add
   * @param handlers An initializer list of pointers to handlers that will be now used as a default handler
   * @param timestamp_clock_type timestamp clock type
   * @param timestamp_clock timestamp clock
   * @return a pointer to the logger
   */
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                        std::initializer_list<std::shared_ptr<Handler>> handlers,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock)
  {
    return create_logger(logger_name, std::vector<std::shared_ptr<Handler>>{handlers},
                         timestamp_clock_type, timestamp_clock);
  }

  /**
   * Create a new logger with multiple handler
   * @param logger_name the name of the logger to add
   * @param handlers An initializer list of pointers to handlers that will be now used as a default handler
   * @param timestamp_clock_type timestamp clock type
   * @param timestamp_clock timestamp clock
   * @return a pointer to the logger
   */
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                        std::vector<std::shared_ptr<Handler>> handlers,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock)
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
    return insert_result.first->second.get();
  }

  /**
   * Marks a logger for deletion. The logger will asynchronously be removed by the logging thread
   */
  void remove_logger(Logger* logger)
  {
    logger->invalidate();
    _has_invalidated_loggers.store(true, std::memory_order_release);
  }

  /**
   * Used internally only to enable console colours on "stdout" default console handler
   * which is created by default in this object constructor.
   * This is meant to called before quill:start() and that is checked internally before calling
   * this function.
   */
  QUILL_ATTRIBUTE_COLD void enable_console_colours() const noexcept
  {
    // Get the previous created default stdout handler
    std::shared_ptr<Handler> const stdout_stream_handler =
      _handler_collection.stdout_console_handler("stdout");
    assert(stdout_stream_handler && "stdout_stream_handler can not be nullptr");

    auto const console_handler = reinterpret_cast<ConsoleHandler*>(stdout_stream_handler.get());
    console_handler->enable_console_colours();
  }

  /**
   * Get the root logger pointer
   * @return root logger ptr
   */
  QUILL_NODISCARD Logger* root_logger() const noexcept { return _root_logger; }

  /**
   * Creates or resets the root logger
   */
  QUILL_ATTRIBUTE_COLD void create_root_logger()
  {
    std::lock_guard<std::recursive_mutex> const lock{_rmutex};

    if (!_root_logger)
    {
      // initial root logger creation once
      if (_config.default_handlers.empty())
      {
        // Add the default console handler to the root logger
        std::shared_ptr<Handler> const stdout_stream_handler =
          _handler_collection.stdout_console_handler("stdout");

        if (_config.enable_console_colours)
        {
          static_cast<ConsoleHandler*>(stdout_stream_handler.get())->enable_console_colours();
        }

        _root_logger =
          create_logger(_config.default_logger_name, stdout_stream_handler,
                        _config.default_timestamp_clock_type, _config.default_custom_timestamp_clock);
      }
      else
      {
        _root_logger =
          create_logger(_config.default_logger_name, _config.default_handlers,
                        _config.default_timestamp_clock_type, _config.default_custom_timestamp_clock);
      }
    }
    else
    {
      // this will update the root logger after it is created.
      // we do not invalidate the initial _root_logger pointer, instead we update the logger
      // if configure is called again, we do not want to invalidate the pointer to the root logger
      if (_config.default_handlers.empty())
      {
        // Add the default console handler to the root logger
        std::shared_ptr<Handler> const stdout_stream_handler =
          _handler_collection.stdout_console_handler("stdout");

        if (_config.enable_console_colours)
        {
          static_cast<ConsoleHandler*>(stdout_stream_handler.get())->enable_console_colours();
        }

        // Register the handler, even if it already exists
        _handler_collection.subscribe_handler(stdout_stream_handler);

        // get the pointer to the existing logger
        auto const search = _logger_name_map.find(_root_logger->_logger_details.name());
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
        auto const search = _logger_name_map.find(_root_logger->_logger_details.name());
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

  /**
   * Called by the backend worker thread only to remove any loggers that are marked as
   * invalidated
   * @return true if loggers were removed
   */
  QUILL_NODISCARD bool remove_invalidated_loggers(std::function<bool(void)> const& check_queues_empty)
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

  /**
   * Checks if any invalidated loggers exist
   * @return
   */
  QUILL_NODISCARD bool has_invalidated_loggers() const noexcept
  {
    return _has_invalidated_loggers.load(std::memory_order_acquire);
  }

  /**
   * Get a list of all the active subscribed handlers filtering out the handlers of any invalidated logger
   * The list contains each handler only once regardless the amount of Logger instances using it
   * This is not used for logging by the backend but only in special cases when
   * e.g. it needs to iterate through all handlers for e.g. to flush
   * @param active_handlers_collection active handlers collection
   */
  void active_handlers(std::vector<std::weak_ptr<Handler>>& active_handlers_collection) const
  {
    active_handlers_collection.clear();

    std::lock_guard<std::recursive_mutex> const lock{_rmutex};
    for (auto const& [logger_name, logger_ptr] : _logger_name_map)
    {
      if (logger_ptr->is_invalidated())
      {
        // Skip handlers of invalidated loggers
        continue;
      }

      std::vector<std::shared_ptr<Handler>> const& logger_handlers = logger_ptr->_logger_details.handlers();
      for (std::shared_ptr<Handler> const& handler : logger_handlers)
      {
        auto search_it =
          std::find_if(std::begin(active_handlers_collection), std::end(active_handlers_collection),
                       [handler_ptr = handler.get()](std::weak_ptr<Handler> const& elem)
                       {
                         // no one else can remove the shared pointer as this is only
                         // running on backend thread, lock() will always succeed
                         return elem.lock().get() == handler_ptr;
                       });

        if (search_it == std::end(active_handlers_collection))
        {
          // Add handler
          active_handlers_collection.push_back(handler);
        }
      }
    }
  }

private:
  Config const& _config;
  ThreadContextCollection& _thread_context_collection; /**< We need to pass this to each logger */
  HandlerCollection& _handler_collection;              /** Collection of al handlers **/
  Logger* _root_logger{nullptr}; /**< A pointer to the root logger to avoid lookup */
  mutable std::recursive_mutex _rmutex; /**< Thread safe access to logger map, Mutable to have a const get_logger() function  */
  std::unordered_map<std::string, std::unique_ptr<Logger>> _logger_name_map; /**< map from logger name to the actual logger */
  std::atomic<bool> _has_invalidated_loggers{false};
};

} // namespace detail
} // namespace quill