/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Common.h"
#include "quill/core/LogLevel.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace quill
{

// ** Forward Declaration **/
class Handler;

namespace detail
{

    class LoggerManager;

class LoggerDetails
{
public:
  /**
   * Constructor
   */
  LoggerDetails(std::string name, std::shared_ptr<Handler> handler, ClockSourceType timestamp_clock_type)
          : _name(std::move(name)), _clock_source(timestamp_clock_type)
  {
    _handlers.push_back(std::move(handler));
  }

  /**
   * Constructor
   */
  LoggerDetails(std::string name, std::vector<std::shared_ptr<Handler>> handlers, ClockSourceType timestamp_clock_type)
          : _name(std::move(name)), _clock_source(timestamp_clock_type)
  {
    _handlers = std::move(handlers);
  }

  /**
   * Deleted
   */
  LoggerDetails(LoggerDetails const&) = delete;
  LoggerDetails& operator=(LoggerDetails const&) = delete;

  /**
   * Destructor
   */
  ~LoggerDetails() = default;

  /**
   * @return The name of the logger
   */
  QUILL_NODISCARD std::string const& name() const noexcept { return _name; }

  /**
   * @return a vector of all handlers of this logger, called by the backend worker thread
   */
  QUILL_NODISCARD std::vector<std::shared_ptr<Handler>> const& handlers() const noexcept
  {
    return _handlers;
  }

  /**
   * @return the timestamp clock time of this logger
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT ClockSourceType

    clock_source_type() const noexcept
  {
      return _clock_source;
  }

  /**
   * Set the backtrace_flush_log_level.
   * @param backtrace_flush_level Any log messages with equal or higher severity that this
   * will also flush the backtrace logs of this logger
   */
  void set_backtrace_flush_level(LogLevel backtrace_flush_level) noexcept
  {
    _backtrace_flush_level.store(backtrace_flush_level, std::memory_order_relaxed);
  }

  /**
   * @return The current backtrace_flush_level
   */
  QUILL_NODISCARD LogLevel backtrace_flush_level() const noexcept
  {
    return _backtrace_flush_level.load(std::memory_order_relaxed);
  }

private:
    friend class LoggerManager;

  std::string _name;
  std::vector<std::shared_ptr<Handler>> _handlers;
    ClockSourceType _clock_source;
    std::atomic<LogLevel> _backtrace_flush_level{
            LogLevel::None}; /** Updated by the caller thread and read by the backend worker thread */
};
} // namespace detail
} // namespace quill
