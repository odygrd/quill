/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/frontend/Logger.h"

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/UserClock.h"

#include <atomic>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace quill::detail
{
    class LoggerManager
{
public:
        LoggerManager(LoggerManager const &) = delete;

        LoggerManager &operator=(LoggerManager const &) = delete;

        /***/
        QUILL_API static LoggerManager
        &

        instance() noexcept
  {
      static LoggerManager instance;
      return instance;
  }

        /***/
        QUILL_NODISCARD Logger
        *
        get_logger(std::string
        const& logger_name) const
        {
            std::lock_guard<std::recursive_mutex> const lock{_mutex};

            auto const search = _loggers.find(logger_name);

            if (QUILL_UNLIKELY(search == _loggers.cend())) {
                // logger does not exist
                QUILL_THROW(QuillError{std::string{"logger does not exist. name: "} + logger_name});
            }

            return search->second.get();
        }

        /***/
  QUILL_NODISCARD std::unordered_map<std::string, Logger*> get_all_loggers() const
  {
      std::lock_guard<std::recursive_mutex> const lock{_mutex};

    std::unordered_map<std::string, Logger*> logger_names;
      logger_names.reserve(_loggers.size());

      for (auto const &[logger_name, logger_ptr]: _loggers)
    {
        logger_names.emplace(logger_name, logger_ptr.get());
    }

    return logger_names;
  }

        /***/
        template<typename TCallback>
        void for_each_logger(TCallback cb) const
  {
      std::lock_guard<std::recursive_mutex> const lock{_mutex};

      for (auto const &[logger_name, logger_ptr]: _loggers)
    {
        cb(logger_ptr.get());
    }
  }

        /***/
        QUILL_NODISCARD Logger
        *
        create_or_get_logger(std::string
        const& logger_name,
        std::shared_ptr<Handler> handler,
                ClockSourceType
        timestamp_clock_type,
        UserClock *timestamp_clock
        )
  {
      std::lock_guard<std::recursive_mutex> const lock{_mutex};

      if (auto const search = _loggers.find(logger_name); search != std::cend(_loggers)) {
          // already exists
          return search->second.get();
      }

      // create logger
      auto [it, inserted] = _loggers.emplace(
              logger_name,
              std::unique_ptr<Logger>{
                      new Logger(logger_name, std::move(handler), timestamp_clock_type, timestamp_clock)});

      return it->second.get();
  }

        /***/
        QUILL_NODISCARD Logger
        *
        create_or_get_logger(std::string
        const& logger_name,
        std::initializer_list<std::shared_ptr<Handler>> handlers,
                ClockSourceType
        timestamp_clock_type,
        UserClock *timestamp_clock
        )
  {
      std::lock_guard<std::recursive_mutex> const lock{_mutex};

      if (auto const search = _loggers.find(logger_name); search != std::cend(_loggers))
    {
        // already exists
        return search->second.get();
    }

      // create logger
      auto [it, inserted] = _loggers.emplace(
              logger_name,
              std::unique_ptr<Logger>{new Logger(logger_name, handlers, timestamp_clock_type, timestamp_clock)});

      return it->second.get();
  }

        /***/
  void remove_logger(Logger* logger)
  {
    logger->invalidate();
    _has_invalidated_loggers.store(true, std::memory_order_release);
  }

        /***/
        template<typename TCheckQueuesEmpty>
        QUILL_NODISCARD bool remove_invalidated_loggers(TCheckQueuesEmpty check_queues_empty)
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
        std::lock_guard<std::recursive_mutex> const lock{_mutex};
        for (auto it = std::begin(_loggers); it != std::end(_loggers);)
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
              it = _loggers.erase(it);
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

        /***/
  QUILL_NODISCARD bool has_invalidated_loggers() const noexcept
  {
    return _has_invalidated_loggers.load(std::memory_order_acquire);
  }

private:
        LoggerManager() = default;

        ~LoggerManager() = default;

    private:
        std::unordered_map<std::string, std::unique_ptr<Logger>> _loggers;
        mutable std::recursive_mutex _mutex;
  std::atomic<bool> _has_invalidated_loggers{false};
};
} // namespace quill::detail