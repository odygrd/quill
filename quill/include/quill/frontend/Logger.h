/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Common.h"

#include "quill/core/Attributes.h"
#include "quill/core/EncodeDecode.h"
#include "quill/core/LogLevel.h"
#include "quill/core/LoggerDetails.h"
#include "quill/core/QuillError.h"
#include "quill/core/Rdtsc.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/core/UserClock.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace quill
{

namespace detail
{
    class LoggerManager;

    class BackendWorker;
} // namespace detail

/**
 * Thread safe logger.
 * Logger must be obtained from LoggerCollection get_logger(), therefore constructors are private
 */
class alignas(detail::CACHE_LINE_ALIGNED) Logger
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
  void* operator new(size_t i) { return detail::alloc_aligned(i, detail::CACHE_LINE_ALIGNED); }
  void operator delete(void* p) { detail::free_aligned(p); }

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
   * @tparam log_statement_level The log level of the log statement to be logged
   * @return bool if a message can be logged based on the current log level
   */
  template <LogLevel log_statement_level>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT bool should_log() const noexcept
  {
    if constexpr (static_cast<LogLevel>(QUILL_ACTIVE_LOG_LEVEL) > log_statement_level)
    {
      return false;
    }

    return log_statement_level >= log_level();
  }

  /**
   * Checks if the given log_statement_level can be logged by this logger
   * @param log_statement_level The log level of the log statement to be logged
   * @return bool if a message can be logged based on the current log level
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT bool should_log(LogLevel log_statement_level) const noexcept
  {
    if constexpr (QUILL_ACTIVE_LOG_LEVEL > 0)
    {
      if (static_cast<LogLevel>(QUILL_ACTIVE_LOG_LEVEL) > log_statement_level)
      {
        return false;
      }
    }

    return log_statement_level >= log_level();
  }

  /**
   * Push a log message to the spsc queue to be logged by the backend thread.
   * One spsc queue per caller thread. This function is enabled only when all arguments are
   * fundamental types.
   * This is the fastest way possible to log
   * @note This function is thread-safe.
   * @param dynamic_log_level dynamic log level
   * @param fmt_args arguments
   */
  template<typename... Args>
  QUILL_ALWAYS_INLINE_HOT void log(LogLevel dynamic_log_level, MacroMetadata const* macro_metadata, Args&&... fmt_args)
  {
    assert(!_is_invalidated.load(std::memory_order_acquire) && "Invalidated loggers can not log");

    // Store the timestamp of the log statement at the start of the call. This gives more accurate
    // timestamp especially if the queue is full
      uint64_t const timestamp = (_logger_details.clock_source() == ClockSourceType::Tsc)
      ? detail::rdtsc()
      : (_logger_details.clock_source() == ClockSourceType::System)
      ? static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count())
      : _user_clock->now();

    detail::ThreadContext* const thread_context =
            quill::detail::get_local_thread_context<QUILL_QUEUE_TYPE>();

    // For windows also take wide strings into consideration.
#if defined(_WIN32)
    constexpr uint32_t c_string_count = detail::count_c_style_strings<Args...>() +
      detail::count_c_style_wide_strings<Args...>() + detail::count_std_wstring_type<Args...>();
#else
    constexpr uint32_t c_string_count = detail::count_c_style_strings<Args...>();
#endif

    size_t c_string_sizes[(std::max)(c_string_count, static_cast<uint32_t>(1))];

    // Need to reserve additional space as we will be aligning the pointer
      size_t total_size = sizeof(uint64_t) + (sizeof(uintptr_t) * 3) +
      detail::calculate_args_size_and_populate_string_lengths(c_string_sizes, fmt_args...);

    if (dynamic_log_level != LogLevel::None)
    {
      // For the dynamic log level we want to add to the total size to store the dynamic log level
      total_size += sizeof(LogLevel);
    }

    // request this size from the queue
    std::byte* write_buffer =
      thread_context->spsc_queue<QUILL_QUEUE_TYPE>().prepare_write(static_cast<uint32_t>(total_size));

    if constexpr (QUILL_QUEUE_TYPE == detail::QueueType::UnboundedNoMaxLimit)
    {
      assert(write_buffer && "UnboundedNonBlocking will always allocate and get new space");
    }
    else if constexpr ((QUILL_QUEUE_TYPE == detail::QueueType::BoundedNonBlocking) ||
                       (QUILL_QUEUE_TYPE == detail::QueueType::UnboundedDropping))
    {
      if (QUILL_UNLIKELY(write_buffer == nullptr))
      {
        // not enough space to push to queue message is dropped
          thread_context->increment_failure_counter();
        return;
      }
    }
    else if constexpr ((QUILL_QUEUE_TYPE == detail::QueueType::BoundedBlocking) ||
                       (QUILL_QUEUE_TYPE == detail::QueueType::UnboundedBlocking))
    {
      if (QUILL_UNLIKELY(write_buffer == nullptr))
      {
          thread_context->increment_failure_counter();

        do
        {
          if constexpr (QUILL_BLOCKING_QUEUE_RETRY_INTERVAL_NS > 0)
          {
            std::this_thread::sleep_for(std::chrono::nanoseconds{QUILL_BLOCKING_QUEUE_RETRY_INTERVAL_NS});
          }

          // not enough space to push to queue, keep trying
          write_buffer =
            thread_context->spsc_queue<QUILL_QUEUE_TYPE>().prepare_write(static_cast<uint32_t>(total_size));
        } while (write_buffer == nullptr);
      }
    }

    // we have enough space in this buffer, and we will write to the buffer

    // Then write the pointer to the LogDataNode. The LogDataNode has all details on how to
    // deserialize the object. We will just serialize the arguments in our queue, but we need
    // to look up their types to deserialize them

#ifndef NDEBUG
    std::byte const* const write_begin = write_buffer;
#endif

    std::memcpy(write_buffer, &timestamp, sizeof(timestamp));
    write_buffer += sizeof(timestamp);

    std::memcpy(write_buffer, &macro_metadata, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    detail::LoggerDetails const* logger_details = &_logger_details;
    std::memcpy(write_buffer, &logger_details, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

      detail::FormatArgsDecoder const ftf = detail::decode_and_populate_format_args<Args...>;
      std::memcpy(write_buffer, &ftf, sizeof(uintptr_t));
      write_buffer += sizeof(uintptr_t);

    // encode remaining arguments
    detail::encode(write_buffer, c_string_sizes, fmt_args...);

    if (dynamic_log_level != LogLevel::None)
    {
      // write the dynamic log level
      std::memcpy(write_buffer, &dynamic_log_level, sizeof(LogLevel));
      write_buffer += sizeof(LogLevel);
    }

#ifndef NDEBUG
    assert(total_size >= (static_cast<uint32_t>(write_buffer - write_begin)) &&
           "The committed write bytes can not be greater than the requested bytes");
    assert((write_buffer >= write_begin) &&
           "write_buffer should be greater or equal to write_begin");
#endif

      thread_context->spsc_queue<QUILL_QUEUE_TYPE>().finish_write(static_cast<uint32_t>(total_size));
    thread_context->spsc_queue<QUILL_QUEUE_TYPE>().commit_write();
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
    // we do not care about the other fields, except quill::MacroMetadata::Event::InitBacktrace
    static constexpr MacroMetadata macro_metadata{
            "", "", "{}", nullptr, LogLevel::Critical, MacroMetadata::Event::InitBacktrace, false};

    // we pass this message to the queue and also pass capacity as arg
      this->log(LogLevel::None, &macro_metadata, capacity);

    // Also store the desired flush log level
    _logger_details.set_backtrace_flush_level(backtrace_flush_level);
  }

  /**
   * Dump any stored backtrace messages
   */
  void flush_backtrace()
  {
    // we do not care about the other fields, except quill::MacroMetadata::Event::Flush
    static constexpr MacroMetadata macro_metadata{
            "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::FlushBacktrace, false};

    // we pass this message to the queue
      this->log(LogLevel::None, &macro_metadata);
  }

    /**
     * Blocks the caller thread until all log messages until the current timestamp are flushed.
     *
     * The backend thread will flush all loggers and all handlers up to the point (timestamp) that
     * this function was called.
     */

    /**
     * Blocks the caller thread until all log messages up to the current timestamp are flushed
     *
     * The backend thread will call write on all handlers for all loggers up to the point (timestamp)
     * that this function was called.
     *
     * @note This function should only be called when the backend worker is running
     */
    void flush() {
        static constexpr MacroMetadata macro_metadata{
                "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::Flush, false};

        std::atomic<bool> backend_thread_flushed{false};
        std::atomic<bool> *backend_thread_flushed_ptr = &backend_thread_flushed;

        this->log(LogLevel::None, &macro_metadata, reinterpret_cast<uintptr_t>(backend_thread_flushed_ptr));

        // The caller thread keeps checking the flag until the backend thread flushes
        do {
            // wait
            std::this_thread::sleep_for(std::chrono::nanoseconds{100});
        } while (!backend_thread_flushed.load());
  }

  /**
   * @return The name of the logger
   */
  QUILL_NODISCARD std::string const& name() const noexcept { return _logger_details.name(); }

private:
    friend class detail::LoggerManager;

    friend class detail::BackendWorker;

  /**
   * Constructs new logger object
   * @param name the name of the logger
   * @param handler handlers for this logger
   * @param clock_source timestamp clock
   * @param user_clock custom timestamp clock
   */
  Logger(std::string const &name, std::shared_ptr<Handler> handler, ClockSourceType clock_source,
         UserClock *user_clock)
          : _user_clock(user_clock),
            _logger_details(name, std::move(handler), clock_source)
  {
      if ((clock_source == ClockSourceType::User) && !user_clock)
    {
      QUILL_THROW(
        QuillError{"A valid TimestampClock* needs to be provided when TimestampClockType is set to "
                   "Custom. Call 'quill::set_user_clock(...)'"});
    }
  }

  /**
   * Constructs a new logger object with multiple handlers
   */
  Logger(std::string const& name, std::vector<std::shared_ptr<Handler>> const& handlers,
         ClockSourceType clock_source, UserClock *user_clock)
          : _user_clock(user_clock),
            _logger_details(name, handlers, clock_source)

  {
      if ((clock_source == ClockSourceType::User) && !user_clock)
    {
      QUILL_THROW(
        QuillError{"A valid TimestampClock* needs to be provided when TimestampClockType is set to "
                   "Custom. Call 'quill::set_user_clock(...)'"});
    }
  }

  void invalidate() { _is_invalidated.store(true, std::memory_order_release); }

  QUILL_NODISCARD bool is_invalidated() const noexcept
  {
    return _is_invalidated.load(std::memory_order_acquire);
  }

private:
    UserClock *_user_clock{nullptr}; /* A non owned pointer to a custom timestamp clock, valid only when provided */
  detail::LoggerDetails _logger_details;
  std::atomic<LogLevel> _log_level{LogLevel::Info};
  std::atomic<bool> _is_invalidated{false};
};

} // namespace quill
