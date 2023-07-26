/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/MacroMetadata.h"
#include "quill/PatternFormatter.h"
#include "quill/TransitEvent.h"
#include "quill/detail/Serialize.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"
#include "quill/filters/FilterBase.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace quill
{

/**
 * Base class for handlers
 */
class Handler
{
public:
  /**
   * Constructor
   * Uses the default pattern formatter
   */
  Handler() = default;

  /**
   * Destructor
   */
  virtual ~Handler() = default;

  Handler(Handler const&) = delete;
  Handler& operator=(Handler const&) = delete;

  /**
   * Set a custom formatter for this handler
   * @warning This function is not thread safe and should be called before any logging to this handler happens
   * Prefer to set the formatter in the constructor when possible via the FileHandlerConfig
   * @param log_pattern format pattern see PatternFormatter
   * @param time_format defaults to "%H:%M:%S.%Qns"
   * @param timezone defaults to PatternFormatter::Timezone::LocalTime
   */
  QUILL_ATTRIBUTE_COLD void set_pattern(
    std::string const& log_pattern, std::string const& time_format = std::string{"%H:%M:%S.%Qns"},
    Timezone timezone = Timezone::LocalTime)
  {
    _formatter = std::make_unique<PatternFormatter>(log_pattern, time_format, timezone);
  }

  /**
   * Returns the owned formatter by the handler
   * @note: Accessor for backend processing
   * @return reference to the pattern formatter of this handler
   */
  QUILL_ATTRIBUTE_HOT PatternFormatter& formatter() { return *_formatter; }

  /**
   * Logs a formatted log message to the handler
   * @note: Accessor for backend processing
   * @param formatted_log_message input log message to write
   * @param log_event transit event
   */
  QUILL_ATTRIBUTE_HOT virtual void write(fmt_buffer_t const& formatted_log_message,
                                         quill::TransitEvent const& log_event) = 0;

  /**
   * Flush the handler synchronising the associated handler with its controlled output sequence.
   */
  QUILL_ATTRIBUTE_HOT virtual void flush() noexcept = 0;

  /**
   * Executes periodically by the backend thread, providing an opportunity for the user
   * to perform custom tasks. For example, batch committing to a database, or any other
   * desired periodic operations.
   *
   * @note It is recommended to avoid performing heavy operations within this function
   *       as it may adversely affect the performance of the backend thread.
   */
  QUILL_ATTRIBUTE_HOT virtual void run_loop() noexcept {};

  /**
   * Sets a log level filter on the handler. Log statements with higher or equal severity only will be logged
   * @note thread safe
   * @param log_level the log level severity
   */
  void set_log_level(LogLevel log_level) { _log_level.store(log_level, std::memory_order_relaxed); }

  /**
   * Looks up the existing log level filter that was set by set_log_level and returns the current log level
   * @note thread-safe
   * @return the current log level of the log level filter
   */
  QUILL_NODISCARD LogLevel get_log_level() const noexcept
  {
    return _log_level.load(std::memory_order_relaxed);
  }

  /** Filters **/

  /**
   * Adds a new filter for this handler.
   * Filters can be added at any time to the handler.
   * @note: thread-safe
   * @param filter instance of a filter class as unique ptr
   */
  void add_filter(std::unique_ptr<FilterBase> filter);

  /**
   * Apply all registered filters.
   * @note: called internally by the backend worker thread.
   * @return result of all filters
   */
  QUILL_NODISCARD bool apply_filters(char const* thread_id, std::chrono::nanoseconds log_message_timestamp,
                                     LogLevel log_level, MacroMetadata const& metadata,
                                     fmt_buffer_t const& formatted_record);

protected:
  /**< Owned formatter for this handler, we have to use a pointer here since the PatterFormatter
   * must not be moved or copied. We create the default pattern formatter always on init */
  std::unique_ptr<PatternFormatter> _formatter = std::make_unique<PatternFormatter>();

private:
  /** Local Filters for this handler **/
  std::vector<FilterBase*> _local_filters;

  /** Global filter for this handler - protected by a spinlock **/
  std::vector<std::unique_ptr<FilterBase>> _global_filters;
  std::recursive_mutex _global_filters_lock;

  std::atomic<quill::LogLevel> _log_level{LogLevel::TraceL3};

  /** Indicator that a new filter was added **/
  std::atomic<bool> _new_filter{false};
};

} // namespace quill
