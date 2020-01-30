#pragma once

#include <tuple>

#include "fmt/format.h"
#include "quill/detail/record/LogRecordUtilities.h"
#include "quill/detail/record/RecordBase.h"
#include "quill/detail/record/StaticLogRecordInfo.h"

namespace quill::detail
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
   * @param log_line_info
   * @param logger_details
   * @param fmt_args
   */
  explicit LogRecord(StaticLogRecordInfo const* log_line_info, LoggerDetails const* logger_details, FmtArgs&&... fmt_args)
    : _log_line_info(log_line_info),
      _logger_details(logger_details),
      _fmt_args(std::make_tuple(std::forward<FmtArgs>(fmt_args)...))
  {
  }

  /**
   * Destructor
   */
  ~LogRecord() override = default;

  /**
   * @return the size of the object
   */
  [[nodiscard]] size_t size() const noexcept override { return sizeof(*this); }

  /**
   * Process a LogRecord
   */
  void backend_process(uint32_t thread_id, std::function<std::vector<Handler*>()> const&) const noexcept override
  {
    // Forward the record to all of the logger handlers
    for (auto& handler : _logger_details->handlers())
    {
      // lambda to unpack the tuple args
      auto forward_tuple_args_to_formatter = [this, timestamp = rdtsc(), thread_id, &handler](auto... tuple_args) {
        return handler->formatter().format(timestamp, thread_id, _logger_details->name(),
                                           *_log_line_info, tuple_args...);
      };

      // formatted record by the formatter
      fmt::memory_buffer const formatted_record = std::apply(forward_tuple_args_to_formatter, this->_fmt_args);

      // log to the handler
      handler->emit(formatted_record);
    }
  }

private:
  StaticLogRecordInfo const* _log_line_info;
  LoggerDetails const* _logger_details;
  PromotedTupleT _fmt_args;
};

} // namespace quill::detail