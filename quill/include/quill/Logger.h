/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/LogLevel.h"
#include "quill/QuillError.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/ThreadContext.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/events//BacktraceEvent.h"
#include "quill/detail/events/LogRecordEvent.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/TypeTraits.h"
#include "quill/detail/misc/TypeTraitsCopyable.h"
#include "quill/detail/misc/Utilities.h"
#include <atomic>
#include <cstdint>
#include <vector>

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
class alignas(detail::CACHELINE_SIZE) Logger
{
public:
  /**
   * Deleted
   */
  Logger(Logger const&) = delete;
  Logger& operator=(Logger const&) = delete;

  /**
   * We align the logger object to it's own cache line. It shouldn't make much difference as the
   * logger object size is exactly 1 cache line
   */
  void* operator new(size_t i) { return detail::aligned_alloc(detail::CACHELINE_SIZE, i); }
  void operator delete(void* p) { detail::aligned_free(p); }

  /**
   * @return The log level of the logger
   */
  QUILL_NODISCARD LogLevel log_level() const noexcept
  {
    return _log_level.load(std::memory_order_relaxed);
  }

  /**
   * Set the log level of the logger
   * @param log_level The new log level
   */
  void set_log_level(LogLevel log_level)
  {
    if (QUILL_UNLIKELY(log_level == LogLevel::Backtrace))
    {
      QUILL_THROW(QuillError{"LogLevel::Backtrace is only used internally. Please don't use it."});
    }

    _log_level.store(log_level, std::memory_order_relaxed);
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @param log_statement_level The log level of the log statement to be logged
   * @return bool if a log record can be logged based on the current log level
   */
  QUILL_NODISCARD bool should_log(LogLevel log_statement_level) const noexcept
  {
    return log_statement_level >= log_level();
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @tparam log_statement_level The log level of the log statement to be logged
   * @return bool if a log record can be logged based on the current log level
   */
  template <LogLevel log_statement_level>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT bool should_log() const noexcept
  {
    return log_statement_level >= log_level();
  }

  /**
   * Push a log record event to the spsc queue to be logged by the backend thread.
   * One queue per caller thread.
   * @note This function is thread-safe.
   * @param fmt_args format arguments
   */
  template <bool IsBackTraceLogRecord, typename TLogRecordMetadata, typename... FmtArgs>
  QUILL_ALWAYS_INLINE_HOT void log(FmtArgs&&... fmt_args)
  {
    static_assert(
      detail::is_all_tuple_copy_constructible<FmtArgs...>::value,
      "The type must be copy constructible. If the type can not be copy constructed it must"
      "be converted to string on the caller side.");

    // Resolve the type of the record first
    using log_record_event_t =
      quill::detail::LogRecordEvent<IsBackTraceLogRecord, TLogRecordMetadata, FmtArgs...>;

#if !defined(QUILL_MODE_UNSAFE)
    static_assert(detail::is_copyable_v<typename log_record_event_t::RealTupleT>,
                  "Trying to copy an unsafe to copy type. Either tag the object with as copy "
                  "loggable or explicitly format to string before logging.");
#endif

#if defined(QUILL_USE_BOUNDED_QUEUE)
    // emplace to the spsc queue owned by the ctx
    if (QUILL_UNLIKELY(!_thread_context_collection.local_thread_context()->spsc_queue().try_emplace<log_record_event_t>(
          std::addressof(_logger_details), std::forward<FmtArgs>(fmt_args)...)))
    {
      // not enough space to push to queue message is dropped
      _thread_context_collection.local_thread_context()->increment_dropped_message_counter();
    }
#else
    // emplace to the spsc queue owned by the ctx
    _thread_context_collection.local_thread_context()->spsc_queue().emplace<log_record_event_t>(
      std::addressof(_logger_details), std::forward<FmtArgs>(fmt_args)...);
#endif
  }

  /**
   * Init a backtrace for this logger.
   * Stores messages logged with LOG_BACKTRACE in a ring buffer messages and displays them later on demand.
   * @param capacity The max number of messages to store in the backtrace
   * @param backtrace_flush_level If this loggers logs any message higher or equal to this severity level the backtrace will also get flushed.
   * Default level is None meaning the user has to call flush_backtrace explicitly
   */
  void init_backtrace(uint32_t capacity, LogLevel backtrace_flush_level = LogLevel::None)
  {
    // Set the backtrace capacity by sending a command event to the queue
    using event_t = detail::BacktraceEvent;
#if defined(QUILL_USE_BOUNDED_QUEUE)
    // emplace to the spsc queue owned by the ctx, we never drop the dump backtrace message
    bool emplaced{false};
    do
    {
      emplaced = _thread_context_collection.local_thread_context()->spsc_queue().try_emplace<event_t>(
        std::addressof(_logger_details), capacity);
    } while (!emplaced);
#else
    _thread_context_collection.local_thread_context()->spsc_queue().emplace<event_t>(
      std::addressof(_logger_details), capacity);
#endif

    // Also store the desired flush log level
    _logger_details.set_backtrace_flush_level(backtrace_flush_level);
  }

  /**
   * Dump any stored backtrace messages
   */
  void flush_backtrace()
  {
    using event_t = detail::BacktraceEvent;

#if defined(QUILL_USE_BOUNDED_QUEUE)
    // emplace to the spsc queue owned by the ctx, we never drop the dump backtrace message
    bool emplaced{false};
    do
    {
      emplaced = _thread_context_collection.local_thread_context()->spsc_queue().try_emplace<event_t>(
        std::addressof(_logger_details));
    } while (!emplaced);
#else
    _thread_context_collection.local_thread_context()->spsc_queue().emplace<event_t>(std::addressof(_logger_details));
#endif
  }

private:
  friend class detail::LoggerCollection;

  /**
   * Constructs new logger object
   * @param name the name of the logger
   * @param handler handlers for this logger
   * @param thread_context_collection thread context collection reference
   */
  Logger(char const* name, Handler* handler, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(name, handler), _thread_context_collection(thread_context_collection)
  {
  }

  /**
   * Constructs a new logger object with multiple handlers
   */
  Logger(char const* name, std::vector<Handler*> handlers, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(name, std::move(handlers)), _thread_context_collection(thread_context_collection)
  {
  }

private:
  detail::LoggerDetails _logger_details;
  detail::ThreadContextCollection& _thread_context_collection;
  std::atomic<LogLevel> _log_level{LogLevel::Info};
};

#if !(defined(_WIN32) && defined(_DEBUG))
// In MSVC debug mode the class has increased size
static_assert(sizeof(Logger) <= detail::CACHELINE_SIZE, "Logger needs to fit in 1 cache line");
#endif

} // namespace quill