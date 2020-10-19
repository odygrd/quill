/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/PatternFormatter.h"
#include "quill/detail/events/LogRecordMetadata.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/RecursiveSpinlock.h"
#include "quill/filters/LogLevelFilter.h"
#include <algorithm>
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
   * Sets a log level filter on the handler. Log statements with higher or equal severity only will be loged
   * @param log_level the log level severity
   */
  void set_log_level(LogLevel log_level)
  {
    std::lock_guard<detail::RecursiveSpinlock> const lock(_global_filters_lock);
    auto search_log_level_filter_it =
      std::find_if(_global_filters.begin(), _global_filters.end(), [](std::unique_ptr<FilterBase>& filter) {
        return filter->get_filter_name() == LogLevelFilter::filter_name;
      });

    if (search_log_level_filter_it != _global_filters.end())
    {
      // Just update the existing
      auto* log_level_filter = reinterpret_cast<LogLevelFilter*>(search_log_level_filter_it->get());
      log_level_filter->set_log_level(log_level);
    }
    else
    {
      // construct a new filter and add it to global
      auto log_level_filter = std::make_unique<LogLevelFilter>(log_level);
      add_filter(std::move(log_level_filter));
    }
  }

  /**
   * Looks up the existing log level filter that was set by set_log_level and returns the current log level
   * @return the current log level of the log level filter - if any
   */
  QUILL_NODISCARD LogLevel get_log_level() noexcept
  {
    // When set_log_level is called for the first time, it won't exist in _local_filters
    // until apply_filters has run at least once

    auto const search_log_level_filter_it =
      std::find_if(_local_filters.cbegin(), _local_filters.cend(), [](FilterBase const* filter) {
        return filter->get_filter_name() == LogLevelFilter::filter_name;
      });

    if (search_log_level_filter_it != _local_filters.cend())
    {
      return reinterpret_cast<LogLevelFilter*>(*search_log_level_filter_it)->get_log_level();
    }
    else
    {
      // No filter found
      return LogLevel::TraceL3;
    }
  }

  /** Filters **/

  /**
   * Adds a new filter for this handler
   * @param filter instance of a filter class as unique ptr
   */
  void add_filter(std::unique_ptr<FilterBase> filter)
  {
    // Lock and add this filter to our global collection
    std::lock_guard<detail::RecursiveSpinlock> const lock(_global_filters_lock);

    // Check if the same filter already exists
    auto const search_filter_it =
      std::find_if(_global_filters.cbegin(), _global_filters.cend(),
                   [&filter](std::unique_ptr<FilterBase> const& elem_filter) {
                     return elem_filter->get_filter_name() == filter->get_filter_name();
                   });

    if (QUILL_UNLIKELY(search_filter_it != _global_filters.cend()))
    {
      QUILL_THROW(QuillError{"Filter with the same name already exists"});
    }

    _global_filters.push_back(std::move(filter));

    // Indicate a new filter was added - here relaxed is okay as the spinlock will do acq-rel on destruction
    _new_filter.store(true, std::memory_order_relaxed);
  }

  /**
   * Apply all registered filters
   * @return result of all filters
   */
  QUILL_NODISCARD bool apply_filters(char const* thread_id, std::chrono::nanoseconds log_record_timestamp,
                                     detail::LogRecordMetadata const& metadata,
                                     fmt::memory_buffer const& formatted_record)
  {
    // Update our local collection of the filters
    if (QUILL_UNLIKELY(_new_filter.load(std::memory_order_relaxed)))
    {
      // if there is a new filter we have to update
      _local_filters.clear();

      std::lock_guard<detail::RecursiveSpinlock> const lock(_global_filters_lock);
      for (auto const& filter : _global_filters)
      {
        _local_filters.push_back(filter.get());
      }

      // all filters loaded so change to false
      _new_filter.store(false, std::memory_order_relaxed);
    }

    return std::all_of(
      _local_filters.begin(), _local_filters.end(),
      [thread_id, log_record_timestamp, &metadata, &formatted_record](FilterBase* filter_elem) {
        return filter_elem->filter(thread_id, log_record_timestamp, metadata, formatted_record);
      });
  }

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
};

} // namespace quill