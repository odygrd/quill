/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/LogMacroMetadata.h"
#include "quill/PatternFormatter.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/RecursiveSpinlock.h"
#include "quill/filters/FilterBase.h"
#include <atomic>
#include <memory>
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
   * Operator new to align this object to a cache line boundary as we always create it on the heap
   */
  void* operator new(size_t i) { return detail::aligned_alloc(detail::CACHELINE_SIZE, i); }
  void operator delete(void* p) { detail::aligned_free(p); }

  /**
   * Set a custom formatter for this handler
   * @param format_pattern format pattern as QUILL_STRING(...)
   * @param timestamp_format defaults to "%H:%M:%S.%Qns"
   * @param timezone defaults to PatternFormatter::Timezone::LocalTime
   */
  template <typename TConstantString>
  QUILL_ATTRIBUTE_COLD void set_pattern(TConstantString format_pattern,
                                        std::string timestamp_format = std::string{"%H:%M:%S.%Qns"},
                                        Timezone timezone = Timezone::LocalTime)
  {
    _formatter = std::make_unique<PatternFormatter>(format_pattern, timestamp_format, timezone);
  }

  /**
   * Returns the owned formatter by the handler
   * @note: Accessor for backend processing
   * @return reference to the pattern formatter of this handler
   */
  QUILL_ATTRIBUTE_HOT PatternFormatter const& formatter() { return *_formatter; }

  /**
   * Logs a formatted log record to the handler
   * @note: Accessor for backend processing
   * @param formatted_log_record input log record to write
   * @param log_record_timestamp log record timestamp
   * @param log_message_severity the severity of the log message
   */
  QUILL_ATTRIBUTE_HOT virtual void write(fmt::memory_buffer const& formatted_log_record,
                                         std::chrono::nanoseconds log_record_timestamp,
                                         LogLevel log_message_severity) = 0;

  /**
   * Flush the handler synchronising the associated handler with its controlled output sequence.
   */
  QUILL_ATTRIBUTE_HOT virtual void flush() noexcept = 0;

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
  QUILL_NODISCARD bool apply_filters(char const* thread_id, std::chrono::nanoseconds log_record_timestamp,
                                     LogMacroMetadata const& metadata,
                                     fmt::memory_buffer const& formatted_record);

private:
  /**< Owned formatter for this handler, we have to use a pointer here since the PatterFormatter
   * must not be moved or copied. We create the default pattern formatter always on init */
  std::unique_ptr<PatternFormatter> _formatter{std::make_unique<PatternFormatter>()};

  /** Local Filters for this handler **/
  std::vector<FilterBase*> _local_filters;

  /** Global filter for this handler - protected by a spinlock **/
  std::vector<std::unique_ptr<FilterBase>> _global_filters;
  quill::detail::RecursiveSpinlock _global_filters_lock;

  /** Indicator that a new filter was added **/
  alignas(detail::CACHELINE_SIZE) std::atomic<bool> _new_filter{false};

  std::atomic<quill::LogLevel> _log_level{LogLevel::TraceL3};
};

} // namespace quill
