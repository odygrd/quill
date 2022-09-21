/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Utilities.h"
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

class LoggerCollection;

class LoggerDetails
{
public:
  /**
   * Constructor
   * @param name
   */
  LoggerDetails(std::string name, Handler* handler, TimestampClockType timestamp_clock_type)
    : _name(std::move(name)), _timestamp_clock_type(timestamp_clock_type)
  {
    _handlers.push_back(handler);
  }

  /**
   * Constructor
   * @param name
   */
  LoggerDetails(std::string name, std::vector<Handler*> handlers, TimestampClockType timestamp_clock_type)
    : _name(std::move(name)), _timestamp_clock_type(timestamp_clock_type)
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
  QUILL_NODISCARD std::vector<Handler*> const& handlers() const noexcept { return _handlers; }

  /**
   * @return a vector of all handlers of this logger, called by the backend worker thread
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT TimestampClockType timestamp_clock_type() const noexcept
  {
    return _timestamp_clock_type;
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
  friend class detail::LoggerCollection;

  std::string _name;
  std::vector<Handler*> _handlers;
  std::atomic<LogLevel> _backtrace_flush_level{LogLevel::None}; /** Updated by the caller thread and read by the backend worker thread */
  TimestampClockType _timestamp_clock_type;
};
} // namespace detail
} // namespace quill
