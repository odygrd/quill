/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "FilterBase.h"
#include "quill/detail/misc/Attributes.h"
#include <atomic>

namespace quill
{
/**
 * A filter that is used in the handlers to filter out the log level.
 * This filter is used by the backend worker thread and it is also safe to be
 * used by multiple threads to change the log level.
 */
class LogLevelFilter : public FilterBase
{
public:
  /** Unique filter name - use a different name per filter */
  static constexpr const char* filter_name{"LogLevelFilter"};

  /**
   * Constructor - default
   */
  LogLevelFilter() : FilterBase(filter_name){};

  /**
   * Constructor with log level
   * @param log_level the desried log level
   */
  explicit LogLevelFilter(LogLevel log_level) : FilterBase(filter_name), _log_level(log_level){};

  /**
   * Filters the log messages
   * @param metadata log statement metadata
   * @return true if we want to log this statement, false otherwise
   */
  QUILL_NODISCARD bool filter(char const*, std::chrono::nanoseconds,
                              quill::detail::LogRecordMetadata const& metadata,
                              fmt::memory_buffer const&) noexcept override
  {
    if (metadata.level() >= _log_level.load(std::memory_order_relaxed))
    {
      // we can log this log statement
      return true;
    }
    return false;
  }

  /**
   * Sets the log level of the filter
   * @param log_level
   */
  void set_log_level(LogLevel log_level) { _log_level.store(log_level, std::memory_order_relaxed); }

  /**
   * Gets the log level of the filter
   * @return current log level of the filter
   */
  QUILL_NODISCARD LogLevel get_log_level() const noexcept
  {
    return _log_level.load(std::memory_order_relaxed);
  }

private:
  /** Log level of the filer */
  std::atomic<LogLevel> _log_level{LogLevel::TraceL3};
};
} // namespace quill