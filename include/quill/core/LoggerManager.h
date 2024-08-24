/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/PatternFormatterOptions.h"
#include "quill/core/Spinlock.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

QUILL_BEGIN_NAMESPACE

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
  QUILL_EXPORT static LoggerManager& instance() noexcept
  {
    static LoggerManager instance;
    return instance;
  }

  /***/
  QUILL_NODISCARD LoggerBase* get_logger(std::string const& logger_name) const
  {
    LockGuard const lock{_spinlock};
    LoggerBase* logger = _find_logger(logger_name);
    return logger && logger->is_valid_logger() ? logger : nullptr;
  }

  /***/
  QUILL_NODISCARD std::vector<LoggerBase*> get_all_loggers() const
  {
    LockGuard const lock{_spinlock};

    std::vector<LoggerBase*> loggers;

    for (auto const& elem : _loggers)
    {
      // we can not add invalidated loggers as they can be removed at any time
      if (elem->is_valid_logger())
      {
        loggers.push_back(elem.get());
      }
    }

    return loggers;
  }

  /***/
  QUILL_NODISCARD LoggerBase* get_valid_logger() const
  {
    // Retrieves any valid logger without the need for constructing a vector
    LockGuard const lock{_spinlock};

    for (auto const& elem : _loggers)
    {
      // we can not add invalidated loggers as they can be removed at any time
      if (elem->is_valid_logger())
      {
        return elem.get();
      }
    }

    return nullptr;
  }

  /***/
  QUILL_NODISCARD size_t get_number_of_loggers() const noexcept
  {
    LockGuard const lock{_spinlock};
    return _loggers.size();
  }

  /**
   * For backend use only
   */
  template <typename TCallback>
  void for_each_logger(TCallback cb) const
  {
    LockGuard const lock{_spinlock};

    for (auto const& elem : _loggers)
    {
      // Here we do not check for valid_logger() like in get_all_loggers() because this
      // function is only called by the backend
      if (cb(elem.get()))
      {
        // When the callback returns true stop the loop early
        break;
      }
    }
  }

  /***/
  template <typename TLogger>
  LoggerBase* create_or_get_logger(std::string const& logger_name, std::vector<std::shared_ptr<Sink>> sinks,
                                   PatternFormatterOptions const& pattern_formatter_options,
                                   ClockSourceType clock_source, UserClockSource* user_clock)
  {
    LockGuard const lock{_spinlock};

    LoggerBase* logger_ptr = _find_logger(logger_name);

    if (!logger_ptr)
    {
      // If logger pointer is null, create a new logger instance.
      std::unique_ptr<LoggerBase> new_logger{
        new TLogger{logger_name, static_cast<std::vector<std::shared_ptr<Sink>>&&>(sinks),
                    pattern_formatter_options, clock_source, user_clock}};

      _insert_logger(static_cast<std::unique_ptr<LoggerBase>&&>(new_logger));

      // Although we could directly return .get() from the new_logger here,
      // we retain this portion of code for additional safety in case of potential re-lookup of
      // the logger. This section is not performance-critical.
      logger_ptr = _find_logger(logger_name);
    }

    assert(logger_ptr);
    assert(logger_ptr->is_valid_logger());
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

      LockGuard const lock{_spinlock};
      for (auto it = _loggers.begin(); it != _loggers.end();)
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
    auto search_it = std::lower_bound(_loggers.begin(), _loggers.end(), logger->get_logger_name(),
                                      [](std::unique_ptr<LoggerBase> const& a, std::string const& b)
                                      { return a->get_logger_name() < b; });

    _loggers.insert(search_it, static_cast<std::unique_ptr<LoggerBase>&&>(logger));
  }

  /***/
  QUILL_NODISCARD LoggerBase* _find_logger(std::string const& target) const noexcept
  {
    auto search_it = std::lower_bound(_loggers.begin(), _loggers.end(), target,
                                      [](std::unique_ptr<LoggerBase> const& a, std::string const& b)
                                      { return a->get_logger_name() < b; });

    return (search_it != std::end(_loggers) && search_it->get()->get_logger_name() == target)
      ? search_it->get()
      : nullptr;
  }

private:
  std::vector<std::unique_ptr<LoggerBase>> _loggers;
  mutable Spinlock _spinlock;
  std::atomic<bool> _has_invalidated_loggers{false};
};
} // namespace detail

QUILL_END_NAMESPACE