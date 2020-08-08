/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/bundled/invoke/invoke.h"
#include "quill/detail/events/BaseEvent.h"
#include "quill/detail/events/LogRecordMetadata.h"
#include "quill/detail/misc/TypeTraits.h"
#include <memory>
#include <tuple>

namespace quill
{
namespace detail
{
/**
 * For each log statement a LogRecordEvent is produced and pushed to the thread local spsc queue.
 * The backend thread reads events from the queue via a BaseEvent*
 *
 * The LogRecordEvent can be a normal LogRecordEvent or a BacktraceLogRecordEvent depending on
 * the template argument IsBacktraceLogRecord
 */
template <bool IsBacktraceLogRecord, typename TLogRecordMetadata, typename... FmtArgs>
class LogRecordEvent final : public BaseEvent
{
public:
  using PromotedTupleT = std::tuple<PromotedTypeT<FmtArgs>...>;
  using RealTupleT = std::tuple<FmtArgs...>;
  using LogRecordMetadataT = TLogRecordMetadata;

  /**
   * Constructor
   * This is created by the caller every time we want to log a new message
   * To perfectly forward the argument we have to provide a templated constructor
   * @param logger_details logger object details
   * @param fmt_args format arguments
   */
  template <typename... UFmtArgs>
  LogRecordEvent(LoggerDetails const* logger_details, UFmtArgs&&... fmt_args)
    : _logger_details(logger_details), _fmt_args(std::make_tuple(std::forward<UFmtArgs>(fmt_args)...))
  {
  }

  /**
   * Destructor
   */
  ~LogRecordEvent() override = default;

  /**
   * Virtual clone
   * @return a copy of this object
   */
  QUILL_NODISCARD std::unique_ptr<BaseEvent> clone() const override
  {
    return std::make_unique<LogRecordEvent>(*this);
  }

  /**
   * @return the size of the object
   */
  QUILL_NODISCARD size_t size() const noexcept override { return sizeof(*this); }

  /**
   * Process a log record or a backtrace log record
   * @param backtrace_log_record_storage The storage mapping of all backtrace records
   * @param thread_id The thread_id that the log record originated
   * @param obtain_active_handlers A callback to obtain all the active handlers for all loggers
   * @param timestamp_callback A callback to obtain the real timestamp of the record
   */
  void backend_process(BacktraceLogRecordStorage& backtrace_log_record_storage,
                       char const* thread_id, GetHandlersCallbackT const& obtain_active_handlers,
                       GetRealTsCallbackT const& timestamp_callback) const override
  {
    // Since this is C++14 with no constexpr and a virtual function we call the internal to
    // branch between processing a backtrace log record or a normal log record
    _backend_process(backtrace_log_record_storage, thread_id, obtain_active_handlers, timestamp_callback);
  }

  /**
   * Process a backtrace record and forward it to all handlers
   * @param thread_id The thread id that initialised the original record. This is stored and provided seperately.
   * @param timestamp_callback A callback to obtain the real timestamp of the record
   */
  void backend_process_backtrace_log_record(char const* thread_id, GetHandlersCallbackT const&,
                                            GetRealTsCallbackT const& timestamp_callback) const override
  {
    // Get the log record timestamp and convert it to a real timestamp in nanoseconds from epoch
    std::chrono::nanoseconds const log_record_timestamp = timestamp_callback(this);

    // Get the metadata of this record
    constexpr detail::LogRecordMetadata log_record_metadata = LogRecordMetadataT{}();

    // Write record to all handlers
    _write_record_to_handlers(thread_id, log_record_timestamp, log_record_metadata);
  }

private:
  /**
   * Process a regular LogRecord
   * @param backtrace_log_record_storage The storage mapping of all backtrace records
   * @param thread_id The thread_id that the log record originated
   * @param obtain_active_handlers A callback to obtain all the active handlers for all loggers
   * @param timestamp_callback A callback to obtain the real timestamp of the record
   */
  template <bool BacktraceLogRecord = IsBacktraceLogRecord>
  std::enable_if_t<!BacktraceLogRecord, void> _backend_process(
    BacktraceLogRecordStorage& backtrace_log_record_storage, char const* thread_id,
    GetHandlersCallbackT const& obtain_active_handlers, GetRealTsCallbackT const& timestamp_callback) const
  {
    // Get the log record timestamp and convert it to a real timestamp in nanoseconds from epoch
    std::chrono::nanoseconds const log_record_timestamp = timestamp_callback(this);

    // Get the metadata of this record
    constexpr detail::LogRecordMetadata log_record_metadata = LogRecordMetadataT{}();

    // Write record to all handlers
    _write_record_to_handlers(thread_id, log_record_timestamp, log_record_metadata);

    // Check if we should also flush the backtrace messages:
    // After we forwarded the message we will check the severity of this message for this logger
    // If the severity of the message is higher than the backtrace flush severity we will also
    // flush the backtrace of the logger
    if (QUILL_UNLIKELY(log_record_metadata.level() >= _logger_details->backtrace_flush_level()))
    {
      // process all records in backtrace for this logger_name and log them by calling backend_process_backtrace_log_record
      // note: we don't use obtain_active_handlers inside backend_process_backtrace_log_record,
      // we only use the handlers of the logger, but we just have to pass it because of the API
      backtrace_log_record_storage.process(
        _logger_details->name(),
        [&obtain_active_handlers, &timestamp_callback](
          std::string const& stored_thread_id, BaseEvent const* stored_backtrace_log_record) {
          stored_backtrace_log_record->backend_process_backtrace_log_record(
            stored_thread_id.data(), obtain_active_handlers, timestamp_callback);
        });
    }
  }

  /**
   * Process a Backtrace LogRecord
   * @param backtrace_log_record_storage The storage mapping of all backtrace records
   * @param thread_id The thread_id that the log record originated
   */
  template <bool BacktraceLogRecord = IsBacktraceLogRecord>
  std::enable_if_t<BacktraceLogRecord, void> _backend_process(BacktraceLogRecordStorage& backtrace_log_record_storage,
                                                              char const* thread_id, GetHandlersCallbackT const&,
                                                              GetRealTsCallbackT const&) const
  {
    // Backtrace record we just store it in the queue
    backtrace_log_record_storage.store(_logger_details->name(), thread_id, this->clone());
  }

  /**
   * Forward this log record to all handlers of this logger
   * @param thread_id The thread_id that the log record originated
   * @param log_record_timestamp the timestamp of the log record since epoch
   * @param log_record_metadata the log record metadata
   */
  void _write_record_to_handlers(char const* thread_id, std::chrono::nanoseconds log_record_timestamp,
                                 detail::LogRecordMetadata const& log_record_metadata) const
  {
    // Forward the record to all of the logger handlers
    for (auto& handler : _logger_details->handlers())
    {
      // lambda to unpack the tuple args stored in the LogRecordEvent (the arguments that were passed by
      // the user) We also capture all additional information we need to create the log message
      auto forward_tuple_args_to_formatter = [this, &log_record_metadata, log_record_timestamp,
                                              thread_id, handler](auto const&... tuple_args) {
        handler->formatter().format(log_record_timestamp, thread_id, _logger_details->name(),
                                    log_record_metadata, tuple_args...);
      };

      // formatted record by the formatter
      invoke_hpp::apply(forward_tuple_args_to_formatter, this->_fmt_args);

      // After calling format on the formatter we have to request the formatter record
      auto const& formatted_log_record_buffer = handler->formatter().formatted_log_record();

      // log to the handler, also pass the log_record_timestamp this is only needed in some
      // cases like daily file rotation
      handler->write(formatted_log_record_buffer, log_record_timestamp);
    }
  }

private:
  LoggerDetails const* _logger_details;
  PromotedTupleT _fmt_args;
};
} // namespace detail
} // namespace quill