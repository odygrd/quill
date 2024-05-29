/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/LogLevel.h"

#include <atomic>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace quill
{
/** Forward Declarations **/
class Sink;
class PatternFormatter;
class UserClockSource;

namespace detail
{
class BackendWorker;

/***/
class LoggerBase
{
public:
  /***/
  LoggerBase(std::string logger_name, std::vector<std::shared_ptr<Sink>> sinks,
             std::string format_pattern, std::string time_pattern, Timezone timezone,
             ClockSourceType clock_source, UserClockSource* user_clock)
    : format_pattern(static_cast<std::string&&>(format_pattern)),
      time_pattern(static_cast<std::string&&>(time_pattern)),
      logger_name(static_cast<std::string&&>(logger_name)),
      user_clock(user_clock),
      timezone(timezone),
      clock_source(clock_source)
  {
#ifndef NDEBUG
    for (auto const& sink : sinks)
    {
      assert(sink && "sink pointer is nullptr");
    }
#endif

    this->sinks = static_cast<std::vector<std::shared_ptr<Sink>>&&>(sinks);
  }

  /***/
  LoggerBase(LoggerBase const&) = delete;
  LoggerBase& operator=(LoggerBase const&) = delete;

  /***/
  virtual ~LoggerBase() = default;

  /**
   * Returns the name of the logger.
   * @return A constant reference to the logger's name.
   */
  QUILL_NODISCARD std::string const& get_logger_name() const noexcept { return logger_name; }

  /**
   * This function sets the logger's validity flag to false, indicating that the logger is no longer valid.
   */
  void mark_invalid() { valid.store(false, std::memory_order_release); }

  /**
   * @brief Checks if the logger is valid.
   * @return True if the logger is valid, false otherwise.
   */
  QUILL_NODISCARD bool is_valid_logger() const noexcept
  {
    return valid.load(std::memory_order_acquire);
  }

  /**
   * @return The log level of the logger
   */
  QUILL_NODISCARD LogLevel get_log_level() const noexcept
  {
    return log_level.load(std::memory_order_relaxed);
  }

  /**
   * Set the log level of the logger
   * @param new_log_level The new log level
   */
  void set_log_level(LogLevel new_log_level)
  {
    if (QUILL_UNLIKELY(new_log_level == LogLevel::Backtrace))
    {
      QUILL_THROW(QuillError{"LogLevel::Backtrace is only used internally. Please don't use it."});
    }

    log_level.store(new_log_level, std::memory_order_relaxed);
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @return bool if a message can be logged based on the current log level
   */
  template <LogLevel log_statement_level>
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool should_log_message() const noexcept
  {
    return log_statement_level >= get_log_level();
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @param log_statement_level The log level of the log statement to be logged
   * @return bool if a message can be logged based on the current log level
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool should_log_message(LogLevel log_statement_level) const noexcept
  {
    return log_statement_level >= get_log_level();
  }

protected:
  friend class detail::BackendWorker;

  std::shared_ptr<PatternFormatter> pattern_formatter; /* The backend thread will set this once, we never access it on the frontend */
  std::string format_pattern; /* Set by the frontend and accessed by the backend to initialise PatternFormatter */
  std::string time_pattern; /* Set by the frontend and accessed by the backend to initialise PatternFormatter */
  std::string logger_name; /* Set by the frontend, accessed by the frontend AND backend */
  std::vector<std::shared_ptr<Sink>> sinks; /* Set by the frontend and accessed by the backend */
  UserClockSource* user_clock{nullptr}; /* A non owned pointer to a custom timestamp clock, valid only when provided. used by frontend only */
  Timezone timezone; /* Set by the frontend and accessed by the backend to initialise PatternFormatter */
  ClockSourceType clock_source; /* Set by the frontend and accessed by the frontend AND backend */

  std::atomic<LogLevel> log_level{LogLevel::Info}; /* used by frontend only */
  std::atomic<LogLevel> backtrace_flush_level{LogLevel::None}; /** Updated by the frontend at any time, accessed by the backend */
  std::atomic<bool> valid{true}; /* Updated by the frontend at any time, accessed by the backend */
};
} // namespace detail
} // namespace quill
