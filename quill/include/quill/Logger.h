#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include "quill/LogLevel.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/record/LogRecord.h"

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
  [[nodiscard]] bool should_log(LogLevel log_statement_level) const noexcept
  {
    return log_statement_level >= log_level();
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @tparam log_statement_level
   * @return
   */
  template <LogLevel log_statement_level>
  [[nodiscard]] bool should_log() const noexcept
  {
    return log_statement_level >= log_level();
  }

  /**
   * Push a log record to the spsc queue to be logged by the backend thread.
   * One queue per caller thread.
   * @note This function is thread-safe.
   */
  template <LogLevel log_statement_level, typename... FmtArgs>
  inline void log(detail::StaticLogRecordInfo const* log_line_info, FmtArgs&&... fmt_args)
  {
    // optimised branches for anything above info
    if constexpr (log_statement_level == LogLevel::TraceL3 || log_statement_level == LogLevel::TraceL2 ||
                  log_statement_level == LogLevel::TraceL1 || log_statement_level == LogLevel::Debug)
    {
      // it is usually unlikely we log those levels
      if (QUILL_LIKELY(!should_log<log_statement_level>()))
        return;
    }
    else
    {
      if (QUILL_UNLIKELY(!should_log<log_statement_level>()))
        return;
    }

    // Resolve the type of the record first
    using log_record_t = quill::detail::LogRecord<FmtArgs...>;

    // emplace to the spsc queue owned by the ctx
    bool retry;
    do
    {
      retry = _thread_context_collection.local_thread_context()->spsc_queue().try_emplace<log_record_t>(
        log_line_info, std::addressof(_logger_details), std::forward<FmtArgs>(fmt_args)...);
      // unlikely case if the queue gets full we will wait until we can log
    } while (QUILL_UNLIKELY(!retry));
  }

private:
  friend class detail::LoggerCollection;

  /**
   * Constructs new logger object
   * @param logger_id A unique id per logger
   * @param log_level The log level of the logger
   */
  Logger(std::string name, Handler* handler, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(std::move(name), handler), _thread_context_collection(thread_context_collection)
  {
  }

  /**
   * Constructs a new logger object with multiple handlers
   * @param name
   * @param handlers
   * @param thread_context_collection
   */
  Logger(std::string name, std::vector<Handler*> handlers, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(std::move(name), std::move(handlers)),
      _thread_context_collection(thread_context_collection)
  {
  }

private:
  detail::LoggerDetails _logger_details;
  detail::ThreadContextCollection& _thread_context_collection;
  std::atomic<LogLevel> _log_level{LogLevel::Info};
};

} // namespace quill