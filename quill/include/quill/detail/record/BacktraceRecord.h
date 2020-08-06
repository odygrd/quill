/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/bundled/invoke/invoke.h"
#include "quill/detail/misc/TypeTraits.h"
#include "quill/detail/record/LogRecordMetadata.h"
#include "quill/detail/record/RecordBase.h"
#include <tuple>

namespace quill
{
namespace detail
{
/**
 * For backtrace log statements a BacktraceRecord is produced and pushed to the thread local spsc
 * queue. The backend thread will retrieve the BacktraceRecords from the queue using the RecordBase
 * class pointer but instead of forwarding them to the handlers it will store them to a ring buffer
 */
template <typename TLogRecordMetadata, typename... FmtArgs>
class BacktraceRecord final : public RecordBase
{
public:
  using PromotedTupleT = std::tuple<PromotedTypeT<FmtArgs>...>;
  using RealTupleT = std::tuple<FmtArgs...>;
  using LogRecordMetadataT = TLogRecordMetadata;

  /**
   * Make a new BacktraceRecord.
   * This is created by the caller every time we want to log a new message
   * To perfectly forward the argument we have to provide a templated constructor
   * @param logger_details logger object details
   * @param fmt_args format arguments
   */
  template <typename... UFmtArgs>
  BacktraceRecord(LoggerDetails const* logger_details, UFmtArgs&&... fmt_args)
    : _logger_details(logger_details), _fmt_args(std::make_tuple(std::forward<UFmtArgs>(fmt_args)...))
  {
  }

  /**
   * Destructor
   */
  ~BacktraceRecord() override = default;

  /**
   * Virtual clone
   * @return a copy of this object
   */
  QUILL_NODISCARD std::unique_ptr<RecordBase> clone() const override
  {
    return std::make_unique<BacktraceRecord>(*this);
  }

  /**
   * @return the size of the object
   */
  QUILL_NODISCARD size_t size() const noexcept override { return sizeof(*this); }

  /**
   * Process the BacktraceRecord
   */
  void backend_process(BacktraceRecordStorage& backtrace_record_storage, char const* thread_id,
                       GetHandlersCallbackT const&, GetRealTsCallbackT const&) const override
  {
    // Store this message to our backtrace queue
    backtrace_record_storage.store(_logger_details->name(), thread_id, this->clone());
  }

  /**
   * Process the backtrace record
   * @param thread_id
   * @param log_record_timestamp
   */
  void backend_process_backtrace_record(char const* thread_id, GetHandlersCallbackT const&,
                                        GetRealTsCallbackT const& timestamp_callback) const override
  {
    // Get the log record timestamp and convert it to a real timestamp in nanoseconds from epoch
    std::chrono::nanoseconds const log_record_timestamp = timestamp_callback(this);

    // Forward the record to all of the logger handlers
    for (auto& handler : _logger_details->handlers())
    {
      // lambda to unpack the tuple args stored in the LogRecord (the arguments that were passed by
      // the user) We also capture all additional information we need to create the log message
      auto forward_tuple_args_to_formatter = [this, log_record_timestamp, thread_id,
                                              handler](auto const&... tuple_args) {
        constexpr detail::LogRecordMetadata log_line_info = LogRecordMetadataT{}();

        handler->formatter().format(log_record_timestamp, thread_id, _logger_details->name(),
                                    log_line_info, tuple_args...);
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