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
#include "quill/core/Metric.h"
#include "quill/core/Rdtsc.h"
#include "quill/core/ThreadPrimitives.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

QUILL_BEGIN_NAMESPACE

#if defined(_WIN32) && defined(_MSC_VER)
  // silence msvc warning C4702: unreachable code
  #pragma warning(push)
  #pragma warning(disable : 4702)
#endif

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
#endif

    write_buffer = _encode_header(
      write_buffer, PackedQword{current_timestamp, reinterpret_cast<uintptr_t>(macro_metadata)},
      PackedQword{reinterpret_cast<uintptr_t>(this), reinterpret_cast<uintptr_t>(detail::decoder_ptr<Args...>)});

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

    _commit_log_statement_reservation(reservation.bounded_queue,
                                      reservation.writer_pos + total_size, enable_immediate_flush);
    return true;
  }

  /**
   * Push a compact metric sample to the spsc queue to be processed by the backend thread.
   * The queue stores only the value. Static metric identity and labels are carried by
   * MetricMetadata via the existing MacroMetadata pointer in the event header.
   *
   * Metrics never participate in logger immediate-flush behavior; sinks decide their own flush
   * or batching strategy on the backend thread.
   *
   * @note This function is thread-safe.
   * @param metric_metadata metadata of the metric event
   * @param value metric value
   *
   * @return true if the metric sample is written to the queue, false if it is dropped
   */
  QUILL_ATTRIBUTE_HOT bool publish_metric(MetricMetadata const* metric_metadata, double value)
  {
    QUILL_ASSERT(metric_metadata != nullptr,
                 "publish_metric() requires a valid MetricMetadata pointer");

    QUILL_ASSERT(metric_metadata->event() == MacroMetadata::Event::Metric,
                 "publish_metric() should only be called with MacroMetadata::Event::Metric");

    QUILL_ASSERT(_valid.load(std::memory_order_acquire),
                 "Attempting to log with an invalidated logger");

    if (_clock_source != ClockSourceType::Tsc)
    {
      return _publish_metric_noinline(metric_metadata, 0, value);
    }

    uint64_t const current_timestamp = detail::rdtsc();
    detail::ThreadContext* const thread_context = _thread_context;

    if (QUILL_UNLIKELY(thread_context == nullptr))
    {
      return _publish_metric_noinline(metric_metadata, current_timestamp, value);
    }

    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    size_t total_size = s_packed_header_size;

    auto const reservation = queue.prepare_write_reserve_cached(total_size);

    if (QUILL_UNLIKELY(reservation.write_buffer == nullptr))
    {
      return _publish_metric_noinline(metric_metadata, current_timestamp, value);
    }

    std::byte* write_buffer = reservation.write_buffer;

#if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)
    std::byte const* const write_begin = write_buffer;
#endif

    // Since the decoder ptr is unused in the header we can use it to store the value
    PackedQword pq;
    pq.lo = reinterpret_cast<uintptr_t>(this);
    std::memcpy(&pq.hi, &value, sizeof(pq.hi));

    write_buffer = _encode_header(
      write_buffer, PackedQword{current_timestamp, reinterpret_cast<uintptr_t>(metric_metadata)}, pq);

    QUILL_ASSERT_WITH_FMT(
      write_buffer > write_begin,
      "write_buffer must be greater than write_begin after encoding in publish_metric(): "
      "metric_source=\"%s\"",
      metric_metadata->source_location());
    QUILL_ASSERT_WITH_FMT(
      total_size == static_cast<size_t>(write_buffer - write_begin),
      "Encoded bytes mismatch in publish_metric(): total_size=%zu, actual_encoded=%zu, "
      "metric_source=\"%s\"",
      total_size, static_cast<size_t>(write_buffer - write_begin), metric_metadata->source_location());

    queue.finish_and_commit_write_reservation(reservation.writer_pos + total_size);
    return true;
  }

  /**
   * Sets or replaces one or more MDC fields for the calling thread.
   *
   * The supplied fields are propagated asynchronously to the backend thread and then appended to
   * subsequent log messages from the same frontend thread until erased or cleared.
   *
   * This operation is not on the hot path. If a dropping queue is configured and is temporarily
   * full, this function retries until the control event is queued.
   */
  template <typename... Args>
  void set_mdc(Args&&... args)
  {
    static_assert(sizeof...(Args) > 0, "logger->set_mdc(...) expects key, value pairs");
    static_assert(
      (sizeof...(Args) % 2u) == 0u,
      "logger->set_mdc(...) expects an even number of arguments: key, value, key, value...");

    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::MdcSet};

    while (!this->template log_statement<false>(&macro_metadata, static_cast<Args&&>(args)...))
    {
      detail::sleep_for_ns(100);
    }
  }

  /**
   * Erases one or more MDC fields for the calling thread.
   *
   * Missing keys are ignored. The operation retries until queued when a dropping queue is
   * temporarily full.
   */
  template <typename... Keys>
  void erase_mdc(Keys&&... keys)
  {
    static_assert(sizeof...(Keys) > 0, "logger->erase_mdc(...) expects one or more keys");

    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::MdcErase};

    while (!this->template log_statement<false>(&macro_metadata, static_cast<Keys&&>(keys)...))
    {
      detail::sleep_for_ns(100);
    }
  }

  /**
   * Clears all MDC fields for the calling thread.
   *
   * The operation retries until queued when a dropping queue is temporarily full.
   */
  void clear_mdc()
  {
    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::MdcClear};

    while (!this->template log_statement<false>(&macro_metadata))
    {
      detail::sleep_for_ns(100);
    }
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
#endif

    write_buffer = _encode_header(
      write_buffer, PackedQword{current_timestamp, reinterpret_cast<uintptr_t>(macro_metadata)},
      PackedQword{reinterpret_cast<uintptr_t>(this), reinterpret_cast<uintptr_t>(detail::decoder_ptr<Args...>)});

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

    _commit_log_statement(queue, total_size, enable_immediate_flush);

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
    if (QUILL_UNLIKELY(max_capacity == 0))
    {
      QUILL_THROW(QuillError{"logger->init_backtrace(...) requires max_capacity > 0"});
    }

    // we do not care about the other fields, except MacroMetadata::Event::InitBacktrace
    static constexpr MacroMetadata macro_metadata{
      "", "", "{}", nullptr, LogLevel::Critical, MacroMetadata::Event::InitBacktrace};

    // we pass this message to the queue and also pass capacity as arg
    // We do not want to drop the message if a dropping queue is used
    while (!this->template log_statement<false>(&macro_metadata, max_capacity))
    {
      detail::sleep_for_ns(100);
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
      detail::sleep_for_ns(100);
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
    if (QUILL_UNLIKELY(detail::LoggerBase::is_current_thread_backend_thread()))
    {
      // flush_log() would wait on the backend worker to process the flush event,
      // but we are the backend worker — surface the misuse instead of self-deadlocking.
      QUILL_THROW(
        QuillError{"logger->flush_log() cannot be called from the backend worker thread"});
    }

    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::Flush};

    std::atomic<bool> backend_thread_flushed{false};
    std::atomic<bool>* backend_thread_flushed_ptr = &backend_thread_flushed;

    // We do not want to drop the message if a dropping queue is used
    while (!this->log_statement<false>(&macro_metadata, reinterpret_cast<uintptr_t>(backend_thread_flushed_ptr)))
    {
      if (sleep_duration_ns > 0)
      {
        detail::sleep_for_ns(sleep_duration_ns);
      }
      else
      {
        detail::yield_thread();
      }
    }

    // The caller thread keeps checking the flag until the backend thread flushes
    while (!backend_thread_flushed.load())
    {
      if (sleep_duration_ns > 0)
      {
        detail::sleep_for_ns(sleep_duration_ns);
      }
      else
      {
        detail::yield_thread();
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
  static_assert(
    sizeof(double) == sizeof(uint64_t),
    "publish_metric packs a double into the uint64_t decoder slot of the packed header");

  static constexpr size_t s_packed_header_size = 2 * sizeof(PackedQword);

  /***/
  LoggerImpl(std::string logger_name, std::vector<std::shared_ptr<Sink>> sinks,
             PatternFormatterOptions pattern_formatter_options, ClockSourceType clock_source,
             UserClockSource* user_clock)
    : detail::LoggerBase(
        static_cast<std::string&&>(logger_name), static_cast<std::vector<std::shared_ptr<Sink>>&&>(sinks),
        static_cast<PatternFormatterOptions&&>(pattern_formatter_options),
        user_clock ? ClockSourceType::User : clock_source, user_clock)
  {
  }

  /**
   * Slow path for publish_metric. Mirrors _log_statement_noinline but uses a decoder that simply
   * consumes the queued metric value without involving the fmt argument store.
   */
  QUILL_NODISCARD QUILL_NOINLINE bool _publish_metric_noinline(MetricMetadata const* metric_metadata,
                                                               uint64_t current_timestamp, double value)
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

    size_t total_size = s_packed_header_size;

    std::byte* write_buffer = _reserve_queue_space(queue, total_size, metric_metadata, thread_context);

    if (QUILL_UNLIKELY(write_buffer == nullptr))
    {
      return false;
    }

#if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)
    std::byte const* const write_begin = write_buffer;
#endif

    PackedQword pq;
    pq.lo = reinterpret_cast<uintptr_t>(this);
    std::memcpy(&pq.hi, &value, sizeof(pq.hi));

    write_buffer = _encode_header(
      write_buffer, PackedQword{current_timestamp, reinterpret_cast<uintptr_t>(metric_metadata)}, pq);

    QUILL_ASSERT_WITH_FMT(write_buffer > write_begin,
                          "write_buffer must be greater than write_begin after encoding in "
                          "_publish_metric_noinline(): metric_source=\"%s\"",
                          metric_metadata->source_location());
    QUILL_ASSERT_WITH_FMT(total_size == static_cast<size_t>(write_buffer - write_begin),
                          "Encoded bytes mismatch in _publish_metric_noinline(): total_size=%zu, "
                          "actual_encoded=%zu, metric_source=\"%s\"",
                          total_size, static_cast<size_t>(write_buffer - write_begin),
                          metric_metadata->source_location());

    queue.finish_and_commit_write(total_size);

    return true;
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
#endif

    write_buffer = _encode_header(
      write_buffer, PackedQword{current_timestamp, reinterpret_cast<uintptr_t>(macro_metadata)},
      PackedQword{reinterpret_cast<uintptr_t>(this),
                  reinterpret_cast<uintptr_t>(detail::decoder_ptr<OriginalArgs...>)});

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

    _commit_log_statement(queue, total_size, enable_immediate_flush);

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
      return detail::get_system_time_ns();
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
        // Not enough space to push: bump the shared failure counter for both log and metric
        // drops so that neither silently disappears. Metrics reuse the existing counter rather
        // than adding a parallel one; the error-notifier message reflects the combined count.
        (void)macro_metadata;
        thread_context->increment_failure_counter();
      }
    }
    else if constexpr (frontend_options_t::queue_type == QueueType::BoundedBlocking)
    {
      if (QUILL_UNLIKELY(write_buffer == nullptr))
      {
        if (QUILL_UNLIKELY(total_size > queue.capacity()))
        {
          QUILL_THROW(
            QuillError{"Logging a single message larger than the configured bounded "
                       "queue capacity is not possible.\n"
                       "Message size: " +
                       std::to_string(total_size) +
                       " bytes\n"
                       "Configured bounded queue capacity: " +
                       std::to_string(queue.capacity()) + " bytes"});
        }

        // See the dropping-queue branch above for why both log and metric events bump the counter.
        (void)macro_metadata;
        thread_context->increment_failure_counter();

        do
        {
          if constexpr (frontend_options_t::blocking_queue_retry_interval_ns > 0)
          {
            detail::sleep_for_ns(frontend_options_t::blocking_queue_retry_interval_ns);
          }

          // not enough space to push to queue, keep trying
          write_buffer = queue.prepare_write(total_size);
        } while (write_buffer == nullptr);
      }
    }
    else if constexpr (frontend_options_t::queue_type == QueueType::UnboundedBlocking)
    {
      if (QUILL_UNLIKELY(write_buffer == nullptr))
      {
        // See the dropping-queue branch above for why both log and metric events bump the counter.
        (void)macro_metadata;
        thread_context->increment_failure_counter();

        do
        {
          if constexpr (frontend_options_t::blocking_queue_retry_interval_ns > 0)
          {
            detail::sleep_for_ns(frontend_options_t::blocking_queue_retry_interval_ns);
          }

          // not enough space to push to queue, keep trying
          write_buffer = queue.prepare_write(total_size);
        } while (write_buffer == nullptr);
      }
    }

    return write_buffer;
  }

  /**
   * Commit a log statement to the queue and optionally flush it.
   *
   * `enable_immediate_flush` is intentionally a runtime parameter (not a template) to avoid
   * one template instantiation per TU per value. At every call site it is a compile-time
   * constant (a non-type template parameter on the caller), so the optimizer constant-folds
   * the branch and emits the same code as the templated version.
   *
   * @param queue Reference to the SPSC queue to commit to
   * @param total_size The total size in bytes of the committed log message
   * @param enable_immediate_flush Whether to honor per-logger immediate-flush thresholds
   */
  QUILL_ATTRIBUTE_HOT inline void _commit_log_statement(queue_t& queue, size_t total_size, bool enable_immediate_flush)
  {
    queue.finish_and_commit_write(total_size);

    _flush_after_log_statement_if_needed(enable_immediate_flush);
  }

  /**
   * Commit a log statement using the reservation-based API.
   * Uses finish_and_commit_write_reservation which takes the new writer position directly,
   * avoiding a re-read of _writer_pos.
   *
   * Takes the BoundedSPSCQueue* cached in the reservation instead of going through
   * UnboundedSPSCQueue::_producer. This saves a pointer reload after the encode stores:
   * the compiler cannot prove the encode does not alias _producer, so without this it
   * re-reads _producer from the ThreadContext at the commit site.
   *
   * See `_commit_log_statement` for why `enable_immediate_flush` is a runtime parameter.
   */
  QUILL_ATTRIBUTE_HOT inline void _commit_log_statement_reservation(detail::BoundedSPSCQueue* bounded_queue,
                                                                    size_t new_writer_pos, bool enable_immediate_flush)
  {
    bounded_queue->finish_and_commit_write_reservation(new_writer_pos);

    _flush_after_log_statement_if_needed(enable_immediate_flush);
  }

  /**
   * Applies the per-logger immediate-flush policy after a log statement has been committed.
   *
   * Backend callbacks are allowed to log, but the backend thread cannot wait for itself to process
   * a flush event. In that case the message remains queued and is processed after the callback
   * returns to the backend loop.
   */
  QUILL_ATTRIBUTE_HOT inline void _flush_after_log_statement_if_needed(bool enable_immediate_flush)
  {
    if (!enable_immediate_flush)
    {
      return;
    }

    uint32_t const threshold = _message_flush_threshold.load(std::memory_order_relaxed);
    if (QUILL_LIKELY(threshold == 0))
    {
      return;
    }

    uint32_t const prev = _messages_since_last_flush.fetch_add(1, std::memory_order_relaxed);
    if ((prev + 1) >= threshold)
    {
      _messages_since_last_flush.store(0, std::memory_order_relaxed);

      // Skip the implicit flush on the backend thread so generic logging code reused
      // there (e.g. backend hooks, custom sinks) does not throw via flush_log().
      if (QUILL_LIKELY(!detail::LoggerBase::is_current_thread_backend_thread()))
      {
        this->flush_log();
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

#if defined(_WIN32) && defined(_MSC_VER)
  #pragma warning(pop)
#endif

QUILL_END_NAMESPACE
