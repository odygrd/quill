/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/bundled/invoke/invoke.h"
#include "quill/detail/misc/TypeTraits.h"
#include "quill/detail/record/RecordBase.h"
#include "quill/detail/record/StaticLogRecordInfo.h"
#include <tuple>

namespace quill
{
namespace detail
{
/**
 * For each log statement a LogRecord is produced and pushed to the thread local spsc queue.
 * The backend thread will retrieve the LogRecords from the queue using the RecordBase class pointer.
 */
template <typename... FmtArgs>
class LogRecord final : public RecordBase
{
public:
  using PromotedTupleT = std::tuple<PromotedTypeT<FmtArgs>...>;
  using RealTupleT = std::tuple<FmtArgs...>;

  /**
   * Deleted
   */
  LogRecord(LogRecord const&) = delete;
  LogRecord& operator=(LogRecord const&) = delete;

  /**
   * Make a new LogRecord.
   * This is created by the caller every time we want to log a new message
   * To perfectly forward the argument we have to provide a templated contructor
   * @param log_line_info log line info constexpr object
   * @param logger_details logger object details
   * @param fmt_args format arguments
   */
  template <typename... UFmtArgs>
  LogRecord(StaticLogRecordInfo const* log_line_info, LoggerDetails const* logger_details, UFmtArgs&&... fmt_args)
    : _log_line_info(log_line_info),
      _logger_details(logger_details),
      _fmt_args(std::make_tuple(std::forward<UFmtArgs>(fmt_args)...))
  {
  }

  /**
   * Destructor
   */
  ~LogRecord() override = default;

  /**
   * @return the size of the object
   */
  QUILL_NODISCARD size_t size() const noexcept override { return sizeof(*this); }

  /**
   * Process a LogRecord
   */
  void backend_process(char const* thread_id,
                       std::function<std::vector<Handler*>()> const&,
                       std::chrono::nanoseconds log_record_timestamp) const noexcept override
  {
    // Forward the record to all of the logger handlers
    for (auto& handler : _logger_details->handlers())
    {
      // lambda to unpack the tuple args stored in the LogRecord (the arguments that were passed by
      // the user) We also capture all additional information we need to create the log message
      auto forward_tuple_args_to_formatter = [this, log_record_timestamp, thread_id, handler](auto... tuple_args) {
        handler->formatter().format(log_record_timestamp, thread_id, _logger_details->name(),
                                    *_log_line_info, tuple_args...);
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
  StaticLogRecordInfo const* _log_line_info;
  LoggerDetails const* _logger_details;
  PromotedTupleT _fmt_args;
};
} // namespace detail
} // namespace quill