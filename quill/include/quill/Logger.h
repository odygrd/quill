#pragma once

#include <atomic>
#include <cstdint>

#include "quill/LogLevel.h"
#include "quill/detail/LogLineInfo.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/message/LogMessage.h"
#include "quill/sinks/SinkBase.h"

namespace quill
{

namespace detail
{
class LoggerCollection;
}

/**
 * Thread safe logger.
 * Logger must be obtained from LoggerCollection get_logger(), therefore constructors are private
 */
class Logger
{
public:
  /**
   * Deleted
   */
  Logger(Logger const&) = delete;
  Logger& operator=(Logger const&) = delete;

  /**
   * @return The log level of the logger
   */
  [[nodiscard]] LogLevel log_level() const noexcept
  {
    return _log_level.load(std::memory_order_relaxed);
  }

  /**
   * Set the log level of the logger
   * @param log_level The new log level
   */
  void set_log_level(LogLevel log_level) noexcept
  {
    _log_level.store(log_level, std::memory_order_relaxed);
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @param log_statement_level The log level of the log statement to be logged
   * @return
   */
  [[nodiscard]] bool can_log(LogLevel log_statement_level) const noexcept
  {
    return log_statement_level >= log_level();
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @tparam log_statement_level
   * @return
   */
  template <LogLevel log_statement_level>
  [[nodiscard]] bool can_log() const noexcept
  {
    return log_statement_level >= log_level();
  }

  /**
   * Push a log message to the spsc queue to be logged by the logging thread.
   * One queue per caller thread.
   * @note This function is thread-safe.
   */
  template <LogLevel log_statement_level, typename... FmtArgs>
  inline void log(detail::LogLineInfo const* log_line_info, FmtArgs&&... fmt_args)
  {
    // optimised branches for anything above info
    if constexpr (log_statement_level == LogLevel::TraceL3 || log_statement_level == LogLevel::TraceL2 ||
                  log_statement_level == LogLevel::TraceL1 || log_statement_level == LogLevel::Debug)
    {
      // it is usually unlikely we log those levels
      if (QUILL_LIKELY(!can_log<log_statement_level>()))
        return;
    }
    else
    {
      if (QUILL_UNLIKELY(!can_log<log_statement_level>()))
        return;
    }

    // Resolve the type of the message first
    using LogMessageT = quill::detail::LogMessage<FmtArgs...>;

    // emplace to the spsc queue owned by the ctx
    bool retry;
    do
    {
      retry = _thread_context_collection.get_local_thread_context()->spsc_queue().try_emplace<LogMessageT>(
        log_line_info, std::addressof(_logger_details), std::forward<FmtArgs>(fmt_args)...);
      // unlikely case if the queue gets full we will wait until we can log
    } while (QUILL_UNLIKELY(!retry));
  }

private:
  friend class detail::LoggerCollection;

  /**
   * Constructs new logger object
   * For efficiency we store an id instead of a string to the logger. This is because we want
   * to avoid copying a string to the spsc queue for every log statement
   * @param logger_id A unique id per logger
   * @param log_level The log level of the logger
   */
  Logger(std::string name, std::unique_ptr<detail::SinkBase> sink, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(std::move(name), std::move(sink)), _thread_context_collection(thread_context_collection)
  {
  }

private:
  detail::LoggerDetails _logger_details;
  detail::ThreadContextCollection& _thread_context_collection;
  std::atomic<LogLevel> _log_level{LogLevel::Info};
};
} // namespace quill