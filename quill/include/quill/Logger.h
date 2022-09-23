/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/LogLevel.h"
#include "quill/QuillError.h"
#include "quill/clock/TimestampClock.h"
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/Serialize.h"
#include "quill/detail/ThreadContext.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Rdtsc.h"
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
class LogManager;
} // namespace detail

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
   * @tparam log_statement_level The log level of the log statement to be logged
   * @return bool if a message can be logged based on the current log level
   */
  template <LogLevel log_statement_level>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT bool should_log() const noexcept
  {
    if constexpr (QUILL_ACTIVE_LOG_LEVEL > static_cast<uint8_t>(log_statement_level))
    {
      return false;
    }

    return log_statement_level >= log_level();
  }

  /**
   * Push a log message to the spsc queue to be logged by the backend thread.
   * One spsc queue per caller thread. This function is enabled only when all arguments are
   * fundamental types.
   * This is the fastest way possible to log
   * @note This function is thread-safe.
   * @param fmt_args format arguments
   */
  template <typename TMacroMetadata, typename TFormatString, typename... FmtArgs>
  QUILL_ALWAYS_INLINE_HOT void log(TFormatString format_string, FmtArgs&&... fmt_args)
  {
#if FMT_VERSION >= 90000
    static_assert(
      !detail::has_fmt_stream_view_v<FmtArgs...>,
      "fmt::streamed(...) is not supported. In order to make a type formattable via std::ostream "
      "you should provide a formatter specialization inherited from ostream_formatter. "
      "`template <> struct fmt::formatter<T> : ostream_formatter {};");
#endif

#if !defined(QUILL_MODE_UNSAFE)
    {
      // not allowing unsafe copies
      static_assert(detail::are_copyable_v<FmtArgs...>,
                    "Trying to copy an unsafe to copy type. Tag or specialize the type as "
                    "`copy_loggable` or explicitly format the type to string on the caller thread"
                    "prior to logging. See "
                    "https://github.com/odygrd/quill/wiki/8.-User-Defined-Types for more info.");
    }
#endif

    fmt::detail::check_format_string<std::remove_reference_t<FmtArgs>...>(format_string);

    detail::ThreadContext* const thread_context = _thread_context_collection.local_thread_context();

    // For windows also take wide strings into consideration.
#if defined(_WIN32)
    constexpr size_t c_string_count = fmt::detail::count<detail::is_type_of_c_string<FmtArgs>()...>() +
      fmt::detail::count<detail::is_type_of_wide_c_string<FmtArgs>()...>() +
      fmt::detail::count<detail::is_type_of_wide_string<FmtArgs>()...>();
#else
    constexpr size_t c_string_count = fmt::detail::count<detail::is_type_of_c_string<FmtArgs>()...>();
#endif

    size_t c_string_sizes[(std::max)(c_string_count, static_cast<size_t>(1))];

    // Need to reserve additional space as we will be aligning the pointer
    size_t const total_size = sizeof(detail::Header) + alignof(detail::Header) +
      detail::get_args_sizes<0>(c_string_sizes, fmt_args...);

    // request this size from the queue
    std::byte* write_buffer = thread_context->spsc_queue().prepare_write(total_size);

#if defined(QUILL_USE_BOUNDED_QUEUE)
    if (QUILL_UNLIKELY(write_buffer == nullptr))
    {
      // not enough space to push to queue message is dropped
      thread_context->increment_dropped_message_counter();
      return;
    }
#endif

    // we have enough space in this buffer, and we will write to the buffer

    // Then write the pointer to the LogDataNode. The LogDataNode has all details on how to
    // deserialize the object. We will just serialize the arguments in our queue but we need to
    // look up their types to deserialize them

    // Note: The metadata variable here is created during program init time,
    std::byte* const write_begin = write_buffer;
    write_buffer = detail::align_pointer<alignof(detail::Header), std::byte>(write_buffer);

    new (write_buffer) detail::Header(
      get_metadata_ptr<TMacroMetadata, FmtArgs...>, std::addressof(_logger_details),
      (_logger_details.timestamp_clock_type() == TimestampClockType::Rdtsc) ? quill::detail::rdtsc()
        : (_logger_details.timestamp_clock_type() == TimestampClockType::System)
        ? static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count())
        : _custom_timestamp_clock->now());

    write_buffer += sizeof(detail::Header);

    // encode remaining arguments
    write_buffer = detail::encode_args<0>(c_string_sizes, write_buffer, std::forward<FmtArgs>(fmt_args)...);
    assert(total_size >= (static_cast<size_t>(write_buffer - write_begin)) &&
           "The committed write bytes can not be greater than the requested bytes");
    thread_context->spsc_queue().commit_write(static_cast<size_t>(write_buffer - write_begin));
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
    struct
    {
      constexpr quill::MacroMetadata operator()() const noexcept
      {
        return quill::MacroMetadata{
          QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__, "{}", LogLevel::Critical, quill::MacroMetadata::Event::InitBacktrace};
      }
    } anonymous_log_message_info;

    // we pass this message to the queue and also pass capacity as arg
    this->template log<decltype(anonymous_log_message_info)>(FMT_STRING("{}"), capacity);

    // Also store the desired flush log level
    _logger_details.set_backtrace_flush_level(backtrace_flush_level);
  }

  /**
   * Dump any stored backtrace messages
   */
  void flush_backtrace()
  {
    // we do not care about the other fields, except quill::MacroMetadata::Event::Flush
    struct
    {
      constexpr quill::MacroMetadata operator()() const noexcept
      {
        return quill::MacroMetadata{
          QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__, "", LogLevel::Critical, quill::MacroMetadata::Event::FlushBacktrace};
      }
    } anonymous_log_message_info;

    // we pass this message to the queue and also pass capacity as arg
    this->template log<decltype(anonymous_log_message_info)>(FMT_STRING(""));
  }

private:
  friend class detail::LoggerCollection;
  friend class detail::LogManager;

  /**
   * Constructs new logger object
   * @param name the name of the logger
   * @param handler handlers for this logger
   * @param timestamp_clock_type timestamp clock
   * @param custom_timestamp_clock custom timestamp clock
   * @param thread_context_collection thread context collection reference
   */
  Logger(std::string const& name, Handler* handler, TimestampClockType timestamp_clock_type,
         TimestampClock* custom_timestamp_clock, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(name, handler, timestamp_clock_type),
      _custom_timestamp_clock(custom_timestamp_clock),
      _thread_context_collection(thread_context_collection)
  {
    if ((timestamp_clock_type == TimestampClockType::Custom) && !custom_timestamp_clock)
    {
      QUILL_THROW(
        QuillError{"A valid TimestampClock* needs to be provided when TimestampClockType is set to "
                   "Custom. Call 'quill::set_custom_timestamp_clock(...)'"});
    }
  }

  /**
   * Constructs a new logger object with multiple handlers
   */
  Logger(std::string const& name, std::vector<Handler*> const& handlers, TimestampClockType timestamp_clock_type,
         TimestampClock* custom_timestamp_clock, detail::ThreadContextCollection& thread_context_collection)
    : _logger_details(name, handlers, timestamp_clock_type),
      _custom_timestamp_clock(custom_timestamp_clock),
      _thread_context_collection(thread_context_collection)
  {
    if ((timestamp_clock_type == TimestampClockType::Custom) && !custom_timestamp_clock)
    {
      QUILL_THROW(
        QuillError{"A valid TimestampClock* needs to be provided when TimestampClockType is set to "
                   "Custom. Call 'quill::set_custom_timestamp_clock(...)'"});
    }
  }

private:
  detail::LoggerDetails _logger_details;
  TimestampClock* _custom_timestamp_clock{nullptr}; /* A non owned pointer to a custom timestamp clock, valid only when provided */
  detail::ThreadContextCollection& _thread_context_collection;
  std::atomic<LogLevel> _log_level{LogLevel::Info};
};

} // namespace quill
