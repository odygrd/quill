/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/QuillError.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace quill
{
/** Forward Declarations **/
class Sink;
class UserClockSource;

namespace detail
{
class LoggerManager
{
public:
  LoggerManager(LoggerManager const&) = delete;
  LoggerManager& operator=(LoggerManager const&) = delete;

  /***/
  QUILL_API static LoggerManager& instance() noexcept
  {
    static LoggerManager instance;
    return instance;
  }

  /***/
  QUILL_NODISCARD LoggerBase* get_logger(std::string const& logger_name) const
  {
    std::lock_guard<std::recursive_mutex> const lock{_mutex};

    LoggerBase* logger = _find_logger(logger_name);

    if (QUILL_UNLIKELY(!logger))
    {
      // logger does not exist
      QUILL_THROW(QuillError{std::string{"logger does not exist. name: "} + logger_name});
    }

    return logger;
  }

  /***/
  QUILL_NODISCARD std::vector<LoggerBase*> get_all_loggers() const
  {
    std::lock_guard<std::recursive_mutex> const lock{_mutex};

    std::vector<LoggerBase*> loggers;
    loggers.reserve(_loggers.size());

    for (auto const& elem : _loggers)
    {
      loggers.push_back(elem.get());
    }

    return loggers;
  }

  /***/
  template <typename TCallback>
  void for_each_logger(TCallback cb) const
  {
    std::lock_guard<std::recursive_mutex> const lock{_mutex};

    for (auto const& elem : _loggers)
    {
      cb(elem.get());
    }
  }

  /***/
  template <typename TLogger>
  QUILL_NODISCARD LoggerBase* create_or_get_logger(std::string const& logger_name,
                                                   std::shared_ptr<Sink> sink, std::string const& format_pattern,
                                                   std::string const& time_pattern, Timezone timestamp_timezone,
                                                   ClockSourceType clock_source, UserClockSource* user_clock)
  {
    std::lock_guard<std::recursive_mutex> const lock{_mutex};

    LoggerBase* logger_ptr = _find_logger(logger_name);

    if (!logger_ptr)
    {
      // If logger pointer is null, create a new logger instance.
      std::unique_ptr<LoggerBase> new_logger{
        new TLogger{logger_name, static_cast<std::shared_ptr<Sink>&&>(sink), format_pattern,
                    time_pattern, timestamp_timezone, clock_source, user_clock}};

      _insert_logger(static_cast<std::unique_ptr<LoggerBase>&&>(new_logger));

      // Although we could directly return .get() from the new_logger here,
      // we retain this portion of code for additional safety in case of potential re-lookup of
      // the logger. This section is not performance-critical.
      logger_ptr = _find_logger(logger_name);
    }

    assert(logger_ptr);
    return logger_ptr;
  }

  /***/
  template <typename TLogger>
  QUILL_NODISCARD LoggerBase* create_or_get_logger(std::string const& logger_name,
                                                   std::initializer_list<std::shared_ptr<Sink>> sinks,
                                                   std::string const& format_pattern,
                                                   std::string const& time_pattern, Timezone timestamp_timezone,
                                                   ClockSourceType clock_source, UserClockSource* user_clock)
  {
    std::lock_guard<std::recursive_mutex> const lock{_mutex};

    LoggerBase* logger_ptr = _find_logger(logger_name);

    if (!logger_ptr)
    {
      // If logger pointer is null, create a new logger instance.
      std::unique_ptr<LoggerBase> new_logger{new TLogger{
        logger_name, sinks, format_pattern, time_pattern, timestamp_timezone, clock_source, user_clock}};

      _insert_logger(static_cast<std::unique_ptr<LoggerBase>&&>(new_logger));

      // Although we could directly return .get() from the new_logger here,
      // we retain this portion of code for additional safety in case of potential re-lookup of
      // the logger. This section is not performance-critical.
      logger_ptr = _find_logger(logger_name);
    }

    assert(logger_ptr);
    return logger_ptr;
  }

  /***/
  void remove_logger(LoggerBase* logger)
  {
    logger->mark_invalid();
    _has_invalidated_loggers.store(true, std::memory_order_release);
  }

  /***/
  template <typename TCheckQueuesEmpty>
  QUILL_NODISCARD std::vector<std::string> cleanup_invalidated_loggers(TCheckQueuesEmpty check_queues_empty)
  {
    std::vector<std::string> removed_loggers;

    if (_has_invalidated_loggers.load(std::memory_order_acquire))
    {
      _has_invalidated_loggers.store(false, std::memory_order_release);

      std::lock_guard<std::recursive_mutex> const lock{_mutex};
      for (auto it = std::begin(_loggers); it != std::end(_loggers);)
      {
        if (!it->get()->is_valid_logger())
        {
          // invalid logger, check if the logger has any pending records in the queue
          if (!check_queues_empty())
          {
            // we have pending records in the queue, we can not remove the logger yet
            ++it;
            _has_invalidated_loggers.store(true, std::memory_order_release);
          }
          else
          {
            removed_loggers.push_back(it->get()->get_logger_name());
            it = _loggers.erase(it);
          }
        }
        else
        {
          ++it;
        }
      }
    }

    return removed_loggers;
  }

  /***/
  QUILL_NODISCARD bool has_invalidated_loggers() const noexcept
  {
    return _has_invalidated_loggers.load(std::memory_order_acquire);
  }

private:
  LoggerManager() = default;
  ~LoggerManager() = default;

  /***/
  void _insert_logger(std::unique_ptr<LoggerBase> logger)
  {
    auto search_it = std::lower_bound(std::begin(_loggers), std::end(_loggers), logger->get_logger_name(),
                                      [](std::unique_ptr<LoggerBase> const& a, std::string const& b)
                                      { return a->get_logger_name() < b; });

    _loggers.insert(search_it, static_cast<std::unique_ptr<LoggerBase>&&>(logger));
  }

  /***/
  QUILL_NODISCARD LoggerBase* _find_logger(std::string const& target) const noexcept
  {
    auto search_it = std::lower_bound(std::begin(_loggers), std::end(_loggers), target,
                                      [](std::unique_ptr<LoggerBase> const& a, std::string const& b)
                                      { return a->get_logger_name() < b; });

    return (search_it != std::end(_loggers) && search_it->get()->get_logger_name() == target)
      ? search_it->get()
      : nullptr;
  }

private:
  std::vector<std::unique_ptr<LoggerBase>> _loggers;
  mutable std::recursive_mutex _mutex;
  std::atomic<bool> _has_invalidated_loggers{false};
};
} // namespace detail
} // namespace quill