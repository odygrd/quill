#pragma once

#include "fmt/format.h"
#include "invoke/invoke.h"
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
 * @tparam FmtArgs
 */
template <typename... FmtArgs>
class LogRecord final : public RecordBase
{
public:
  using PromotedTupleT = std::tuple<PromotedTypeT<FmtArgs>...>;

  /**
   * Deleted
   */
  LogRecord(LogRecord const&) = delete;
  LogRecord& operator=(LogRecord const&) = delete;

  /**
   * Make a new LogRecord.
   * This is created by the caller every time we want to log a new message
   * To perfectly forward the argument we have to provide a templated contructor
   * @param log_line_info
   * @param logger_details
   * @param fmt_args
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
                       RdtscClock const* rdtsc_clock) const noexcept override
  {
    // Forward the record to all of the logger handlers
    for (auto& handler : _logger_details->handlers())
    {
#if defined(QUILL_RDTSC_CLOCK)
      // pass to our clock the stored rdtsc from the caller
      std::chrono::nanoseconds const timestamp = rdtsc_clock->time_since_epoch(this->timestamp());
#else
      // Then the timestamp() will be already in epoch no need to convert it like above
      // The precision of system_clock::time-point is not portable across platforms.
      std::chrono::system_clock::duration const timestamp_duration{this->timestamp()};
      std::chrono::nanoseconds const timestamp = std::chrono::nanoseconds{timestamp_duration};
#endif

      // lambda to unpack the tuple args stored in the LogRecord (the arguments that were passed by
      // the user) We also capture all additional information we need to create the log message
      auto forward_tuple_args_to_formatter = [this, timestamp, thread_id, handler](auto... tuple_args) {
        handler->formatter().format(timestamp, thread_id, _logger_details->name(), *_log_line_info, tuple_args...);
      };

      // formatted record by the formatter
      invoke_hpp::apply(forward_tuple_args_to_formatter, this->_fmt_args);

      // After calling format on the formatter we have to request the formatter record
      auto const& formatted_log_record_buffer = handler->formatter().formatted_log_record();

      // log to the handler
      handler->emit(formatted_log_record_buffer);
    }
  }

private:
  StaticLogRecordInfo const* _log_line_info;
  LoggerDetails const* _logger_details;
  PromotedTupleT _fmt_args;
};
} // namespace detail
} // namespace quill