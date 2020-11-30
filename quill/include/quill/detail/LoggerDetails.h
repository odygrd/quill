/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Utilities.h"
#include "quill/handlers/Handler.h"
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace quill
{
namespace detail
{
/**
 * The logger object is broken down to LoggerDetails and Logger as we end up in circular include
 * references if we include both in LogRecord
 *
 * Logger includes LogEvent as it needs it to create it, and LogEvent needs to read the
 * LoggerDetails later during the backend thread processing, but we don't want to include Logger
 */
class LoggerDetails
{
public:
  /**
   * Constructor
   * @param name
   */
  LoggerDetails(char const* name, Handler* handler)
  {
    safe_strncpy(_name, name);
    _handlers.push_back(handler);
  }

  /**
   * Constructor
   * @param name
   */
  LoggerDetails(char const* name, std::vector<Handler*> handlers)
  {
    safe_strncpy(_name, name);
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
  QUILL_NODISCARD char const* name() const noexcept { return _name.data(); }

  /**
   * @return a vector of all handlers of this logger, called by the backend worker thread
   */
  QUILL_NODISCARD std::vector<Handler*> const& handlers() const noexcept { return _handlers; }

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
  std::array<char, 22> _name; /** Because size of string in gcc is 32 we use an array here as we need the Logger object to fit all in a single cache line */
  std::atomic<LogLevel> _backtrace_flush_level{
    LogLevel::None}; /** Updated by the caller thread and read by the backedn worker thread */
  std::vector<Handler*> _handlers;
};
} // namespace detail
} // namespace quill