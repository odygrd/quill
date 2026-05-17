/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/UserClockSource.h"
#include "quill/core/Attributes.h"
#include "quill/core/ChronoTimeUtils.h"
#include "quill/core/Codec.h"
#include "quill/core/Common.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/LogLevel.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/Rdtsc.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

QUILL_BEGIN_NAMESPACE

/** Forward Declarations **/
QUILL_BEGIN_EXPORT

class Sink;

QUILL_END_EXPORT

namespace detail
{
class LoggerManager;

class BackendWorker;
} // namespace detail

QUILL_BEGIN_EXPORT

/**
 * @brief Thread safe logger.
 *
 * Logger must be obtained from create_or_get_logger(), therefore, constructors are private
 */
template <typename TFrontendOptions>
class LoggerImpl : public detail::LoggerBase
{
public:
  using frontend_options_t = TFrontendOptions;

  static constexpr bool using_unbounded_queue =
    (frontend_options_t::queue_type == QueueType::UnboundedBlocking) ||
    (frontend_options_t::queue_type == QueueType::UnboundedDropping);

  using queue_t =
    std::conditional_t<using_unbounded_queue, detail::UnboundedSPSCQueue, detail::BoundedSPSCQueue>;

  /***/
  LoggerImpl(LoggerImpl const&) = delete;

  LoggerImpl& operator=(LoggerImpl const&) = delete;

  ~LoggerImpl() override = default;

  /**
   * Push a log message to the spsc queue to be logged by the backend thread.
   * One spsc queue per caller thread. This function is enabled only when all arguments are
   * fundamental types.
   * This is the fastest way possible to log
   * @note This function is thread-safe.
   * @param macro_metadata metadata of the log message
   * @param fmt_args arguments
   *
   * @return true if the message is written to the queue, false if it is dropped (when a dropping queue is used)
   */
  template <bool enable_immediate_flush, typename... Args>
  QUILL_ATTRIBUTE_HOT bool log_statement(MacroMetadata const* macro_metadata, Args&&... fmt_args)
  {
    QUILL_ASSERT(
      macro_metadata->event() != MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy &&
        macro_metadata->event() != MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy &&
        macro_metadata->event() != MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy,
      "log_statement() should not be called with MacroMetadata::Event::LogWithRuntimeMetadata");

    QUILL_ASSERT(_valid.load(std::memory_order_acquire),
                 "Attempting to log with an invalidated logger");

    if (_clock_source != ClockSourceType::Tsc)
    {
      return _log_statement_noinline<enable_immediate_flush, Args...>(
        macro_metadata, 0, static_cast<Args&&>(fmt_args)...);
    }

    uint64_t const current_timestamp = detail::rdtsc();
    detail::ThreadContext* const thread_context = _thread_context;

    if (QUILL_UNLIKELY(thread_context == nullptr))
    {
      return _log_statement_noinline<enable_immediate_flush, Args...>(
        macro_metadata, current_timestamp, static_cast<Args&&>(fmt_args)...);
    }

    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    size_t total_size = s_packed_header_size +
      detail::compute_encoded_size_and_cache_string_lengths(
                          thread_context->get_conditional_arg_size_cache(), fmt_args...);

    auto const reservation = queue.prepare_write_reserve_cached(total_size);

    if (QUILL_UNLIKELY(reservation.write_buffer == nullptr))
    {
      return _log_statement_noinline<enable_immediate_flush, Args...>(
        macro_metadata, current_timestamp, static_cast<Args&&>(fmt_args)...);
    }

    std::byte* write_buffer = reservation.write_buffer;

#if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)
    std::byte const* const write_begin = write_buffer;
    QUILL_ASSERT(write_begin, "Reserved queue space returned nullptr in log_statement()");
#endif

    write_buffer = _encode_header(
      write_buffer, PackedQword{current_timestamp, reinterpret_cast<uintptr_t>(macro_metadata)},
      PackedQword{reinterpret_cast<uintptr_t>(this),
                  reinterpret_cast<uintptr_t>(detail::decode_and_store_args<detail::remove_cvref_t<Args>...>)});

    detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                   static_cast<decltype(fmt_args)&&>(fmt_args)...);

    QUILL_ASSERT_WITH_FMT(
      write_buffer > write_begin,
      "write_buffer must be greater than write_begin after encoding in log_statement(): msg=\"%s\"",
      macro_metadata->message_format());
    QUILL_ASSERT_WITH_FMT(
      total_size == static_cast<size_t>(write_buffer - write_begin),
      "Encoded bytes mismatch in log_statement(): total_size=%zu, actual_encoded=%zu, msg=\"%s\"",
      total_size, static_cast<size_t>(write_buffer - write_begin), macro_metadata->message_format());

    _commit_log_statement_reservation<enable_immediate_flush>(queue, reservation.writer_pos + total_size);
    return true;
  }

  /**
   * @brief Push a log message with runtime metadata to the spsc queue to be logged by the backend thread.
   *
   * Similar to log_statement but allows passing metadata that is only available at runtime.
   *
   * @note This function is thread-safe.
   * @param macro_metadata Metadata of the log message
   * @param fmt Format string for the log message
   * @param file_path Source file path where the log statement was called
   * @param function_name Function name where the log statement was called
   * @param tags Optional tags associated with the log message
   * @param line_number Line number in the source file
   * @param log_level Severity level of the log message
   * @param fmt_args Format arguments for the log message
   *
   * @return true if the message is written to the queue, false if it is dropped (when a dropping queue is used)
   */
  template <bool enable_immediate_flush, typename... Args>
  QUILL_ATTRIBUTE_HOT bool log_statement_runtime_metadata(MacroMetadata const* macro_metadata,
                                                          char const* fmt, char const* file_path,
                                                          char const* function_name,
                                                          char const* tags, uint32_t line_number,
                                                          LogLevel log_level, Args&&... fmt_args)
  {
    QUILL_ASSERT(macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy ||
                   macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy ||
                   macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy,
                 "log_statement_runtime_metadata() should only be called with "
                 "MacroMetadata::Event::LogWithRuntimeMetadata");

    QUILL_ASSERT(_valid.load(std::memory_order_acquire),
                 "Attempting to log with an invalidated logger");

    uint64_t const current_timestamp =
      (_clock_source == ClockSourceType::Tsc) ? detail::rdtsc() : _get_non_tsc_timestamp();

    if (QUILL_UNLIKELY(_thread_context == nullptr))
    {
      _thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    detail::ThreadContext* const thread_context = _thread_context;
    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    size_t total_size{s_packed_header_size};

    if (macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy)
    {
      total_size += detail::compute_encoded_size_and_cache_string_lengths(
        thread_context->get_conditional_arg_size_cache(), fmt, file_path, function_name, tags,
        line_number, log_level, fmt_args...);
    }
    else if (macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy)
    {
      total_size += detail::compute_encoded_size_and_cache_string_lengths(
        thread_context->get_conditional_arg_size_cache(), static_cast<void const*>(fmt),
        static_cast<void const*>(file_path), static_cast<void const*>(function_name),
        static_cast<void const*>(tags), line_number, log_level, fmt_args...);
    }
    else if (macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy)
    {
      total_size += detail::compute_encoded_size_and_cache_string_lengths(
        thread_context->get_conditional_arg_size_cache(), fmt, static_cast<void const*>(file_path),
        static_cast<void const*>(function_name), tags, line_number, log_level, fmt_args...);
    }
    else
    {
      return false;
    }

    std::byte* write_buffer = _reserve_queue_space(queue, total_size, macro_metadata, thread_context);

    if (QUILL_UNLIKELY(write_buffer == nullptr))
    {
      return false;
    }

#if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)
    std::byte const* const write_begin = write_buffer;
    QUILL_ASSERT(write_begin,
                 "Reserved queue space returned nullptr in log_statement_runtime_metadata()");
#endif

    write_buffer = _encode_header(
      write_buffer, PackedQword{current_timestamp, reinterpret_cast<uintptr_t>(macro_metadata)},
      PackedQword{reinterpret_cast<uintptr_t>(this),
                  reinterpret_cast<uintptr_t>(detail::decode_and_store_args<detail::remove_cvref_t<Args>...>)});

    if (macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy)
    {
      detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(), fmt, file_path, function_name,
                     tags, line_number, log_level, static_cast<decltype(fmt_args)&&>(fmt_args)...);
    }
    else if (macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy)
    {
      detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                     static_cast<void const*>(fmt), static_cast<void const*>(file_path),
                     static_cast<void const*>(function_name), static_cast<void const*>(tags),
                     line_number, log_level, static_cast<decltype(fmt_args)&&>(fmt_args)...);
    }
    else if (macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy)
    {
      detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(), fmt,
                     static_cast<void const*>(file_path), static_cast<void const*>(function_name),
                     tags, line_number, log_level, static_cast<decltype(fmt_args)&&>(fmt_args)...);
    }

    QUILL_ASSERT_WITH_FMT(write_buffer > write_begin,
                          "write_buffer must be greater than write_begin after encoding in "
                          "log_statement_runtime_metadata(): fmt=\"%s\"",
                          fmt);
    QUILL_ASSERT_WITH_FMT(total_size == static_cast<size_t>(write_buffer - write_begin),
                          "Encoded bytes mismatch in log_statement_runtime_metadata(): "
                          "total_size=%zu, actual_encoded=%zu, fmt=\"%s\"",
                          total_size, static_cast<size_t>(write_buffer - write_begin), fmt);

    _commit_log_statement<enable_immediate_flush>(queue, total_size);

    return true;
  }

  /**
   * Init a backtrace for this logger.
   * Stores messages logged with LOG_BACKTRACE in a ring buffer and displays them later on demand.
   * @param max_capacity The max number of messages to store in the backtrace
   * @param flush_level If this logger logs any message higher or equal to this severity level, the backtrace will also get flushed.
   * Default level is None meaning the user has to call flush_backtrace explicitly
   */
  void init_backtrace(uint32_t max_capacity, LogLevel flush_level = LogLevel::None)
  {
    QUILL_ASSERT(max_capacity > 0, "logger->init_backtrace(...) requires max_capacity > 0");

    // we do not care about the other fields, except MacroMetadata::Event::InitBacktrace
    static constexpr MacroMetadata macro_metadata{
      "", "", "{}", nullptr, LogLevel::Critical, MacroMetadata::Event::InitBacktrace};

    // we pass this message to the queue and also pass capacity as arg
    // We do not want to drop the message if a dropping queue is used
    while (!this->template log_statement<false>(&macro_metadata, max_capacity))
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    }

    // Also store the desired flush log level
    _backtrace_flush_level.store(flush_level, std::memory_order_relaxed);
  }

  /**
   * Dump any stored backtrace messages
   */
  void flush_backtrace()
  {
    // we do not care about the other fields, except MacroMetadata::Event::Flush
    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::FlushBacktrace};

    // We do not want to drop the message if a dropping queue is used
    while (!this->template log_statement<false>(&macro_metadata))
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    }
  }

  /**
   * Blocks the calling thread until all log messages up to the current timestamp are flushed.
   *
   * The backend thread will invoke the write operation on all sinks for all loggers up to the point
   * (timestamp) when this function is invoked.
   *
   * @param sleep_duration_ns The duration in nanoseconds to sleep between retries when the
   * blocking queue is full, and between checks for the flush completion. Default is 100 nanoseconds.
   *
   * @warning Do not call this function from the destructor of a static object. This may lead to application crashes if the thread-local ThreadContext is destroyed before the static object invoking `flush_log`.
   * @note This function should only be called when the backend worker is running after Backend::start(...)
   * @note This function will block the calling thread until the flush message is processed by the backend thread.
   *       The calling thread can block for up to backend_options.sleep_duration. If you configure a custom
   *       long sleep duration on the backend thread, e.g., backend_options.sleep_duration = std::chrono::minutes{1},
   *       then you should ideally avoid calling this function as you can block for long period of times unless
   *       you use another thread that calls Backend::notify()
   */
  void flush_log(uint32_t sleep_duration_ns = 100)
  {
    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::Flush};

    std::atomic<bool> backend_thread_flushed{false};
    std::atomic<bool>* backend_thread_flushed_ptr = &backend_thread_flushed;

    // We do not want to drop the message if a dropping queue is used
    while (!this->log_statement<false>(&macro_metadata, reinterpret_cast<uintptr_t>(backend_thread_flushed_ptr)))
    {
      if (sleep_duration_ns > 0)
      {
        std::this_thread::sleep_for(std::chrono::nanoseconds{sleep_duration_ns});
      }
      else
      {
        std::this_thread::yield();
      }
    }

    // The caller thread keeps checking the flag until the backend thread flushes
    while (!backend_thread_flushed.load())
    {
      if (sleep_duration_ns > 0)
      {
        std::this_thread::sleep_for(std::chrono::nanoseconds{sleep_duration_ns});
      }
      else
      {
        std::this_thread::yield();
      }
    }
  }

private:
  friend class detail::LoggerManager;
  friend class detail::BackendWorker;

  /***/
  struct PackedQword
  {
    uint64_t lo;
    uint64_t hi;
  };

  static_assert(sizeof(detail::FormatArgsDecoder) == sizeof(uintptr_t),
                "FormatArgsDecoder must fit in uintptr_t for packed header encoding");
  static_assert(sizeof(uintptr_t) <= sizeof(uint64_t),
                "Packed header encoding requires pointers to fit in 64 bits");

  static constexpr size_t s_packed_header_size = 2 * sizeof(PackedQword);

  /***/
  LoggerImpl(std::string logger_name, std::vector<std::shared_ptr<Sink>> sinks,
             PatternFormatterOptions pattern_formatter_options, ClockSourceType clock_source,
             UserClockSource* user_clock)
    : detail::LoggerBase(
        static_cast<std::string&&>(logger_name), static_cast<std::vector<std::shared_ptr<Sink>>&&>(sinks),
        static_cast<PatternFormatterOptions&&>(pattern_formatter_options), clock_source, user_clock)
  {
    if (this->_user_clock)
    {
      // if a user clock is provided then set the ClockSourceType to User
      this->_clock_source = ClockSourceType::User;
    }
  }

  /**
   * Slow path for log_statement. Handles all cold conditions: non-TSC clock,
   * thread_context initialization, and queue cache miss.
   * Kept NOINLINE so log_statement's hot path avoids a full stack frame.
   * If current_timestamp is 0, a non-TSC timestamp is fetched.
   */
  template <bool enable_immediate_flush, typename... OriginalArgs, typename... Args>
  QUILL_NODISCARD QUILL_NOINLINE bool _log_statement_noinline(MacroMetadata const* macro_metadata,
                                                              uint64_t current_timestamp, Args&&... fmt_args)
  {
    if (current_timestamp == 0)
    {
      current_timestamp = _get_non_tsc_timestamp();
    }

    if (QUILL_UNLIKELY(_thread_context == nullptr))
    {
      _thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    detail::ThreadContext* const thread_context = _thread_context;
    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    size_t total_size = s_packed_header_size +
      detail::compute_encoded_size_and_cache_string_lengths(
                          thread_context->get_conditional_arg_size_cache(), fmt_args...);

    std::byte* write_buffer = _reserve_queue_space(queue, total_size, macro_metadata, thread_context);

    if (QUILL_UNLIKELY(write_buffer == nullptr))
    {
      return false;
    }

#if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)
    std::byte const* const write_begin = write_buffer;
    QUILL_ASSERT(write_begin, "Reserved queue space returned nullptr in _log_statement_noinline()");
#endif

    write_buffer = _encode_header(
      write_buffer, PackedQword{current_timestamp, reinterpret_cast<uintptr_t>(macro_metadata)},
      PackedQword{reinterpret_cast<uintptr_t>(this),
                  reinterpret_cast<uintptr_t>(
                    detail::decode_and_store_args<detail::remove_cvref_t<OriginalArgs>...>)});

    detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                   static_cast<decltype(fmt_args)&&>(fmt_args)...);

    QUILL_ASSERT_WITH_FMT(write_buffer > write_begin,
                          "write_buffer must be greater than write_begin after encoding in "
                          "_log_statement_noinline(): msg=\"%s\"",
                          macro_metadata->message_format());
    QUILL_ASSERT_WITH_FMT(total_size == static_cast<size_t>(write_buffer - write_begin),
                          "Encoded bytes mismatch in _log_statement_noinline(): total_size=%zu, "
                          "actual_encoded=%zu, msg=\"%s\"",
                          total_size, static_cast<size_t>(write_buffer - write_begin),
                          macro_metadata->message_format());

    _commit_log_statement<enable_immediate_flush>(queue, total_size);

    return true;
  }

  /**
   * @brief Non-TSC timestamp path, kept noinline so that the compiler does not pull
   * the System/UserClock loads and branches into the caller.
   */
  QUILL_NODISCARD QUILL_NOINLINE uint64_t _get_non_tsc_timestamp() const
  {
    if (_clock_source == ClockSourceType::System)
    {
      return detail::get_timestamp_ns<std::chrono::system_clock>();
    }

    if (_user_clock)
    {
      return _user_clock->now();
    }

    // not expected
    return 0;
  }

  /**
   * Reserve space in the thread's SPSC queue for a log message
   * @param queue Reference to the SPSC queue to reserve space in
   * @param total_size The total size in bytes needed for the log message
   * @param macro_metadata Metadata of the log message, used to increment failure counter if needed
   * @param thread_context The thread context, used to increment failure counter if needed
   * @return std::byte* Pointer to the reserved space in the queue, or nullptr if space cannot be allocated
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* _reserve_queue_space(queue_t& queue, size_t total_size,
                                                                      MacroMetadata const* macro_metadata,
                                                                      detail::ThreadContext* thread_context)
  {
    std::byte* write_buffer = queue.prepare_write(total_size);

    if constexpr ((frontend_options_t::queue_type == QueueType::BoundedDropping) ||
                  (frontend_options_t::queue_type == QueueType::UnboundedDropping))
    {
      if (QUILL_UNLIKELY(write_buffer == nullptr))
      {
        // not enough space to push to queue message is dropped
        if (macro_metadata->event() == MacroMetadata::Event::Log)
        {
          thread_context->increment_failure_counter();
        }
      }
    }
    else if constexpr ((frontend_options_t::queue_type == QueueType::BoundedBlocking) ||
                       (frontend_options_t::queue_type == QueueType::UnboundedBlocking))
    {
      if (QUILL_UNLIKELY(write_buffer == nullptr))
      {
        if (macro_metadata->event() == MacroMetadata::Event::Log)
        {
          thread_context->increment_failure_counter();
        }

        do
        {
          if constexpr (frontend_options_t::blocking_queue_retry_interval_ns > 0)
          {
            std::this_thread::sleep_for(std::chrono::nanoseconds{frontend_options_t::blocking_queue_retry_interval_ns});
          }

          // not enough space to push to queue, keep trying
          write_buffer = queue.prepare_write(total_size);
        } while (write_buffer == nullptr);
      }
    }

    return write_buffer;
  }

  /**
   * Commit a log statement to the queue and optionally flush it
   * @param queue Reference to the SPSC queue to commit to
   * @param total_size The total size in bytes of the committed log message
   */
  template <bool enable_immediate_flush>
  QUILL_ATTRIBUTE_HOT void _commit_log_statement(queue_t& queue, size_t total_size)
  {
    queue.finish_and_commit_write(total_size);

    if constexpr (enable_immediate_flush)
    {
      uint32_t const threshold = _message_flush_threshold.load(std::memory_order_relaxed);
      if (QUILL_UNLIKELY(threshold != 0))
      {
        uint32_t const prev = _messages_since_last_flush.fetch_add(1, std::memory_order_relaxed);
        if ((prev + 1) >= threshold)
        {
          _messages_since_last_flush.store(0, std::memory_order_relaxed);
          this->flush_log();
        }
      }
    }
  }

  /**
   * Commit a log statement using the reservation-based API.
   * Uses finish_and_commit_write_reservation which takes the new writer position directly,
   * avoiding a re-read of _writer_pos.
   */
  template <bool enable_immediate_flush>
  QUILL_ATTRIBUTE_HOT void _commit_log_statement_reservation(queue_t& queue, size_t new_writer_pos)
  {
    queue.finish_and_commit_write_reservation(new_writer_pos);

    if constexpr (enable_immediate_flush)
    {
      uint32_t const threshold = _message_flush_threshold.load(std::memory_order_relaxed);
      if (QUILL_UNLIKELY(threshold != 0))
      {
        uint32_t const prev = _messages_since_last_flush.fetch_add(1, std::memory_order_relaxed);
        if ((prev + 1) >= threshold)
        {
          _messages_since_last_flush.store(0, std::memory_order_relaxed);
          this->flush_log();
        }
      }
    }
  }

  /**
   * Encodes header information into the write buffer.
   *
   * Header fields are passed as PackedQword pairs (16 bytes each) so that on x86-64 the
   * System V ABI delivers them in XMM registers, enabling the compiler to emit vmovdqu/movups
   * stores instead of individual 8-byte movs.
   *
   * @return Updated pointer to the write buffer after encoding the header.
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT static std::byte* _encode_header(std::byte* write_buffer, PackedQword ts_meta,
                                                                       PackedQword logger_decoder) noexcept
  {
    std::memcpy(write_buffer, &ts_meta, sizeof(ts_meta));
    write_buffer += sizeof(ts_meta);

    std::memcpy(write_buffer, &logger_decoder, sizeof(logger_decoder));
    write_buffer += sizeof(logger_decoder);

    return write_buffer;
  }
};

using Logger = LoggerImpl<FrontendOptions>;

QUILL_END_EXPORT

QUILL_END_NAMESPACE
