#pragma once

#include <tuple>

#include "quill/detail/message/MessageBase.h"

#include "quill/detail/LogLineInfo.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/message/LogMessageHelpers.h"

#include "fmt/format.h"

namespace quill::detail
{
/**
 * For each log statement a message is produced and pushed to the thread local spsc queue.
 * The logging thread will retrieve the messages from the queue using the base class pointer.
 * @tparam TPromotedTuple
 * @tparam U This is unused for this message. We need it for the custom command message
 */
template <typename... FmtArgs>
class LogMessage final : public MessageBase
{
public:
  using PromotedTupleT = std::tuple<PromotedTypeT<FmtArgs>...>;

  /**
   * Deleted
   */
  LogMessage(LogMessage const&) = delete;
  LogMessage& operator=(LogMessage const&) = delete;

  /**
   * Make a new message. Created by the caller
   * @param log_line_info
   * @param logger_details
   * @param fmt_args
   */
  explicit LogMessage(LogLineInfo const* log_line_info, LoggerDetails const* logger_details, FmtArgs&&... fmt_args)
    : _log_line_info(log_line_info),
      _logger_details(logger_details),
      _fmt_args(std::make_tuple(std::forward<FmtArgs>(fmt_args)...))
  {
  }

  /**
   * Destructor
   */
  ~LogMessage() override = default;

  /**
   * @return the size of the object
   */
  [[nodiscard]] size_t size() const noexcept override { return sizeof(*this); }

  /**
   * Process a message
   */
  void backend_process(uint32_t thread_id) const noexcept override
  {
    // Forward the message to all of the logger sinks
    for (auto& sink : _logger_details->sinks())
    {
      // lambda to unpack the tuple args
      auto forward_tuple_args_to_formatter = [this, timestamp = rdtsc(), thread_id, &sink](auto... tuple_args) {
        return sink->formatter().format(timestamp, thread_id, _logger_details->name(),
                                        *_log_line_info, tuple_args...);
      };

      // formatted line by the formatter
      fmt::memory_buffer const formatted_line = std::apply(forward_tuple_args_to_formatter, this->_fmt_args);

      // log to the sink
      sink->log(formatted_line);
    }
  }

private:
  LogLineInfo const* _log_line_info;
  LoggerDetails const* _logger_details;
  PromotedTupleT _fmt_args;
};
} // namespace quill::detail