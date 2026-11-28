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

#ifndef QUILL_MODULE
  #include <atomic>
  #include <cstddef>
  #include <cstdint>
  #include <cstring>
  #include <initializer_list>
  #include <memory>
  #include <string>
  #include <string_view>
  #include <type_traits>
  #include <utility>
  #include <vector>
#endif

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

/**
 * A class rather than an alias of a constexpr function call: a specialisation is instantiated once
 * per argument type, while the call is evaluated again at each use of the alias
 */
template <typename Arg>
struct slow_path_arg
{
private:
  using arg_t = remove_cvref_t<Arg>;

  // The Microsoft x64 ABI passes 16-byte aggregates indirectly, so copying them into a
  // temporary before a noinline call is strictly worse than forwarding the existing reference.
  // System V x64 and AArch64 can pass two-word aggregates in registers.
#if defined(_WIN32) && (defined(__x86_64__) || defined(_M_X64))
  static constexpr size_t max_by_value_size = sizeof(uintptr_t);
#else
  static constexpr size_t max_by_value_size = sizeof(uintptr_t) * 2u;
#endif

  static constexpr bool by_value = std::is_trivially_copyable_v<arg_t> &&
    std::is_trivially_copy_constructible_v<arg_t> && !std::is_array_v<arg_t> &&
    (sizeof(arg_t) <= max_by_value_size);

public:
  using type = std::conditional_t<by_value, arg_t, Arg&&>;
};

/**
 * @param max_size the largest value of size_t
 * @param queue_header_size the size of the header that precedes every record in the queue
 * @return the most bytes of a binary record. Its length is encoded in 32 bits, and the space
 * that is reserved for it in the queue, with the header and the length before the bytes, has to
 * be a size that size_t can hold: on a 32-bit platform it would wrap around otherwise
 */
QUILL_NODISCARD constexpr uint64_t max_binary_record_size(uint64_t max_size, uint64_t queue_header_size) noexcept
{
  uint64_t const max_encoded_length = UINT32_MAX - 1u;
  uint64_t const max_reservable = max_size - queue_header_size - sizeof(uint32_t);
  return (max_reservable < max_encoded_length) ? max_reservable : max_encoded_length;
}

/** Writes the size - 1 bytes of `str`, which may be nullptr, and a null character */
QUILL_ATTRIBUTE_HOT inline void encode_c_string(std::byte*& buffer, char const* str, uint32_t size) noexcept
{
  if (QUILL_LIKELY(str != nullptr))
  {
    std::memcpy(buffer, str, size - 1u);
  }

  buffer[size - 1u] = std::byte{'\0'};
  buffer += size;
}

/**
 * Writes the fields of `metadata` at their offsets. A copy of the whole struct can take a vector
 * register, and its size and padding depend on the ABI, e.g. it is 20 bytes on 32-bit x86
 */
QUILL_ATTRIBUTE_HOT inline void encode_static_runtime_metadata(std::byte*& buffer,
                                                               detail::StaticRuntimeMetadata const& metadata) noexcept
{
  std::memcpy(buffer + offsetof(detail::StaticRuntimeMetadata, fmt), &metadata.fmt, sizeof(metadata.fmt));
  std::memcpy(buffer + offsetof(detail::StaticRuntimeMetadata, file), &metadata.file,
              sizeof(metadata.file));
  std::memcpy(buffer + offsetof(detail::StaticRuntimeMetadata, function), &metadata.function,
              sizeof(metadata.function));
  std::memcpy(buffer + offsetof(detail::StaticRuntimeMetadata, packed), &metadata.packed,
              sizeof(metadata.packed));
  buffer += sizeof(detail::StaticRuntimeMetadata);
}

/**
 * Writes the metadata of a MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy record as the
 * backend decodes it with Codec: the format string with its null character, the file and the
 * function pointers, the tags with their null character, the line and the log level
 * @param fmt_size the length of `fmt` plus one, as is tags_size of `tags`
 */
QUILL_ATTRIBUTE_HOT inline void encode_hybrid_copy_metadata(std::byte*& buffer, char const* fmt, uint32_t fmt_size,
                                                            char const* file, char const* function,
                                                            char const* tags, uint32_t tags_size,
                                                            uint32_t line, LogLevel log_level) noexcept
{
  encode_c_string(buffer, fmt, fmt_size);

  void const* const file_ptr = file;
  void const* const function_ptr = function;
  std::memcpy(buffer, &file_ptr, sizeof(file_ptr));
  buffer += sizeof(file_ptr);
  std::memcpy(buffer, &function_ptr, sizeof(function_ptr));
  buffer += sizeof(function_ptr);

  encode_c_string(buffer, tags, tags_size);

  std::memcpy(buffer, &line, sizeof(line));
  buffer += sizeof(line);
  std::memcpy(buffer, &log_level, sizeof(log_level));
  buffer += sizeof(log_level);
}
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

  static_assert(std::is_base_of_v<FrontendOptions, frontend_options_t>,
                "Custom frontend options must derive from quill::FrontendOptions");

  using clock_type = typename frontend_options_t::clock_type;

  static constexpr bool using_runtime_clock = std::is_same_v<clock_type, ClockSource>;
  using clock_object_t = std::conditional_t<using_runtime_clock, UserClockSource, clock_type>;
  using clock_source_t = std::conditional_t<using_runtime_clock, ClockSource, UserClockRef<clock_type>>;

  static_assert(std::is_base_of_v<UserClockSource, clock_object_t>,
                "FrontendOptions::clock_type must be ClockSource or derive from UserClockSource");

  static constexpr bool using_unbounded_queue =
    (frontend_options_t::queue_type == QueueType::UnboundedBlocking) ||
    (frontend_options_t::queue_type == QueueType::UnboundedDropping);

  using queue_t =
    std::conditional_t<using_unbounded_queue, detail::UnboundedSPSCQueue, detail::BoundedSPSCQueue>;

  /** The most bytes of a binary record, see log_binary() and detail::max_binary_record_size() */
  static constexpr size_t max_binary_record_size{
    static_cast<size_t>(detail::max_binary_record_size(SIZE_MAX, 4u * sizeof(uint64_t)))};

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
   * @return true if the message is written to the queue, false if it is dropped
   */
  template <typename... Args>
  QUILL_ATTRIBUTE_HOT bool log_statement(MacroMetadata const* macro_metadata, Args&&... fmt_args)
  {
    QUILL_ASSERT(
      macro_metadata->event() != MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy &&
        macro_metadata->event() != MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy &&
        macro_metadata->event() != MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy &&
        macro_metadata->event() != MacroMetadata::Event::LogWithStaticRuntimeMetadata,
      "log_statement() should not be called with MacroMetadata::Event::LogWithRuntimeMetadata");

    QUILL_ASSERT(_valid.load(std::memory_order_acquire),
                 "Attempting to log with an invalidated logger");

    // For runtime clock selection, keep non-TSC reads in the noinline helper. A shared writer
    // plus non-TSC wrapper added extra Args... instantiations per signature; joining runtime
    // clock calls into this body instead introduced TSC register saves in our assembly probes.
    if constexpr (using_runtime_clock)
    {
      if (_clock_source != ClockSourceType::Tsc)
      {
        return _log_statement_noinline<Args...>(macro_metadata, 0, nullptr, static_cast<Args&&>(fmt_args)...);
      }
    }

    detail::ThreadContext* const thread_context = _thread_context;

    if (QUILL_UNLIKELY(thread_context == nullptr))
    {
      return _log_statement_noinline<Args...>(macro_metadata, 0, nullptr, static_cast<Args&&>(fmt_args)...);
    }

    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    size_t const total_size = s_header_size +
      detail::compute_encoded_size_and_cache_string_lengths(
                                thread_context->get_conditional_arg_size_cache(), fmt_args...);

    auto const reservation = queue.prepare_write_reserve_cached(total_size);

    if (QUILL_UNLIKELY(reservation.write_buffer == nullptr))
    {
      // Pass the computed size: the size cache already holds this call's encoded arguments
      return _log_statement_noinline<Args...>(macro_metadata, total_size, nullptr,
                                              static_cast<Args&&>(fmt_args)...);
    }

    if (total_size > detail::prefetched_record_min_size)
    {
      // For a fixed-size signature the comparison is constant, so a small record compiles it out
      detail::prefetch_record_for_write(reservation.write_buffer, total_size);
    }

    std::byte* write_buffer = reservation.write_buffer + s_header_size;

    detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                   static_cast<decltype(fmt_args)&&>(fmt_args)...);

    QUILL_ASSERT_WITH_FMT(
      total_size == static_cast<size_t>(write_buffer - reservation.write_buffer),
      "Encoded bytes mismatch in log_statement(): total_size=%zu, actual_encoded=%zu, msg=\"%s\"", total_size,
      static_cast<size_t>(write_buffer - reservation.write_buffer), macro_metadata->message_format());

    _encode_header<true>(reservation.write_buffer, macro_metadata,
                         reinterpret_cast<uintptr_t>(detail::decoder_ptr<Args...>));

    // Commit through the cached queue pointer: encoding may alias UnboundedSPSCQueue::_producer,
    // so looking it up again here would otherwise require a pointer reload.
    reservation.bounded_queue->finish_and_commit_write(total_size);

    _flush_after_log_statement_if_needed(macro_metadata);
    return true;
  }

  /**
   * Push a binary record to the spsc queue to be written by the backend thread.
   *
   * The backend passes the bytes as they are to Sink::write_binary() of each sink of this logger.
   * They are not formatted: the log level, the pattern formatter and the sink filters do not apply
   * to a binary record, and it never triggers an immediate flush. It is ordered by timestamp with
   * the other events.
   *
   * @note This function is thread-safe. The bytes are copied, so `data` may be changed or
   * destroyed after it returns.
   * @param macro_metadata metadata of the call, e.g. its source location, that the sinks get with
   * the bytes. Its event is MacroMetadata::Event::Binary. The queue keeps this pointer, so the
   * metadata and the strings it refers to have to stay valid until the backend has written the
   * record: define it `static constexpr` with string literals, as LOG_BINARY does for its call site
   * @param data the bytes to write
   * @param size the number of bytes, at most max_binary_record_size
   *
   * @throws QuillError if the record is larger than max_binary_record_size
   * @return true if the record is written to the queue, false if it is dropped
   */
  QUILL_ATTRIBUTE_HOT bool log_binary(MacroMetadata const* macro_metadata, void const* data, size_t size)
  {
    QUILL_ASSERT(macro_metadata->event() == MacroMetadata::Event::Binary,
                 "log_binary() should only be called with MacroMetadata::Event::Binary");

    QUILL_ASSERT((data != nullptr) || (size == 0),
                 "log_binary() requires data when size is not zero");

    if (QUILL_UNLIKELY(size > max_binary_record_size))
    {
      _throw_binary_record_too_large();
    }

    // A string view is encoded as its length followed by its bytes
    return this->log_statement(macro_metadata, std::string_view{static_cast<char const*>(data), size});
  }

  /**
   * Push a binary record that is given in two parts, e.g. a header that the caller builds for
   * data it has received. The sinks get one record, the bytes of the header followed by the data,
   * so nothing can come between the two in the output and the data is copied once.
   *
   * See log_binary(MacroMetadata const*, void const*, size_t)
   *
   * @param macro_metadata metadata of the call
   * @param header the bytes that the record starts with
   * @param header_size the number of bytes of the header
   * @param data the bytes that follow the header
   * @param size the number of bytes of the data. The two sizes add up to at most
   * max_binary_record_size
   *
   * @throws QuillError if the record is larger than max_binary_record_size
   * @return true if the record is written to the queue, false if it is dropped
   */
  QUILL_ATTRIBUTE_HOT bool log_binary(MacroMetadata const* macro_metadata, void const* header,
                                      size_t header_size, void const* data, size_t size)
  {
    QUILL_ASSERT(macro_metadata->event() == MacroMetadata::Event::Binary,
                 "log_binary() should only be called with MacroMetadata::Event::Binary");

    QUILL_ASSERT(((header != nullptr) || (header_size == 0)) && ((data != nullptr) || (size == 0)),
                 "log_binary() requires a header and data when their sizes are not zero");

    if (QUILL_UNLIKELY((header_size > max_binary_record_size) || (size > (max_binary_record_size - header_size))))
    {
      _throw_binary_record_too_large();
    }

    return this->log_statement(macro_metadata,
                               detail::BinaryBuffers{header, data, static_cast<uint32_t>(header_size),
                                                     static_cast<uint32_t>(size)});
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

    if constexpr (using_runtime_clock)
    {
      if (_clock_source != ClockSourceType::Tsc)
      {
        return _publish_metric_noinline(metric_metadata, value);
      }
    }

    detail::ThreadContext* const thread_context = _thread_context;

    if (QUILL_UNLIKELY(thread_context == nullptr))
    {
      return _publish_metric_noinline(metric_metadata, value);
    }

    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    constexpr size_t total_size = s_header_size;

    auto const reservation = queue.prepare_write_reserve_cached(total_size);

    if (QUILL_UNLIKELY(reservation.write_buffer == nullptr))
    {
      return _publish_metric_noinline(metric_metadata, value);
    }

    // Since the decoder ptr is unused in the header we can use it to store the value
    uint64_t value_bits;
    std::memcpy(&value_bits, &value, sizeof(value_bits));

    _encode_header<true>(reservation.write_buffer, metric_metadata, value_bits);

    // Commit through the cached queue pointer to avoid reloading the producer node
    reservation.bounded_queue->finish_and_commit_write_reservation(reservation.writer_pos + total_size);
    return true;
  }

  /**
   * Publishes a sample with runtime labels, copying the name and labels into the queue.
   * The backend resolves a stable MetricMetadata for each distinct name/label combination.
   * Unlike publish_metric(), this copies strings on every call. Metadata is retained for the
   * program's lifetime. Metrics do not trigger immediate flushing.
   *
   * @note This function is thread-safe. Input strings may be changed or destroyed after it returns.
   * @return true if queued, false if dropped by the configured queue policy
   */
  bool publish_dynamic_metric(std::string_view metric_name, std::vector<MetricLabel> const& labels, double value)
  {
    return _publish_dynamic_metric(metric_name, labels, value);
  }

  /**
   * Publishes a sample with runtime labels given inline, e.g.
   * `publish_dynamic_metric("requests_total", {{"method", method}, {"status", "200"}}, 1.0)`.
   * Unlike the std::vector<MetricLabel> overload, this does not allocate on the calling thread.
   *
   * @note This function is thread-safe. Input strings may be changed or destroyed after it returns.
   * @return true if queued, false if dropped by the configured queue policy
   */
  bool publish_dynamic_metric(std::string_view metric_name,
                              std::initializer_list<std::pair<std::string_view, std::string_view>> labels,
                              double value)
  {
    return _publish_dynamic_metric(metric_name, labels, value);
  }

  /**
   * Sets or replaces one or more MDC fields for the calling thread.
   *
   * The supplied fields are propagated asynchronously to the backend thread and then appended to
   * subsequent log messages from the same frontend thread until erased or cleared.
   *
   * This operation is not on the hot path. On frontend threads, if a dropping queue is configured
   * and is temporarily full, this function retries until the control event is queued.
   * An event larger than a bounded queue's capacity is rejected with QuillError.
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

    _enqueue_control_event(&macro_metadata, static_cast<Args&&>(args)...);
  }

  /**
   * Erases one or more MDC fields for the calling thread.
   *
   * Missing keys are ignored. On frontend threads, the operation retries until queued when a
   * dropping queue is temporarily full.
   */
  template <typename... Keys>
  void erase_mdc(Keys&&... keys)
  {
    static_assert(sizeof...(Keys) > 0, "logger->erase_mdc(...) expects one or more keys");

    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::MdcErase};

    _enqueue_control_event(&macro_metadata, static_cast<Keys&&>(keys)...);
  }

  /**
   * Clears all MDC fields for the calling thread.
   *
   * On frontend threads, the operation retries until queued when a dropping queue is temporarily
   * full.
   */
  void clear_mdc()
  {
    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::MdcClear};

    _enqueue_control_event(&macro_metadata);
  }

  /**
   * Push a log message of the macro-free functions, see LogFunctions.h, to the spsc queue to be
   * logged by the backend thread.
   *
   * @note This function is thread-safe.
   * @param log_level Severity level of the log message
   * @param format The format string, source location and line of the call. When its
   * `metadata.packed` is not 0 the strings are literals that the record refers to, as the record of
   * a macro refers to its metadata. Otherwise the format string is copied with the tags, as by
   * log_statement_runtime_metadata() with MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy
   * @param tags Tags of the call, or nullptr. A call with tags copies its format string
   * @param fmt_args Format arguments for the log message
   *
   * @return true if the message is written to the queue, false if it is dropped
   */
  template <typename Format, typename... Args>
  QUILL_ATTRIBUTE_HOT bool log_statement_macro_free(LogLevel log_level, Format const& format,
                                                    char const* tags, Args&&... fmt_args)
  {
    auto const& metadata = format.metadata;
    uint32_t const line = format.line;
    QUILL_ASSERT((metadata.packed == 0) || (tags == nullptr),
                 "log_statement_macro_free() copies the format string of a call with tags");

    QUILL_ASSERT(_valid.load(std::memory_order_acquire),
                 "Attempting to log with an invalidated logger");

    if constexpr (Format::supports_literal_records)
    {
      if (metadata.packed != 0)
      {
        // The record of a literal format string is the one of log_statement() with the metadata after
        // the header, and it takes the same slow path. Its metadata holds the level, None for an invalid one
        size_t const level_index = static_cast<size_t>(log_level);
        MacroMetadata const* const macro_metadata =
          &s_static_runtime_metadata[(level_index < LogLevelCount) ? level_index : static_cast<size_t>(LogLevel::None)];

        // The slow path takes a copy of the metadata, which lets the caller keep it in registers when
        // it is a constant: the address of `metadata` itself would have to be taken on the fast path
        if constexpr (using_runtime_clock)
        {
          if (QUILL_UNLIKELY(_clock_source != ClockSourceType::Tsc))
          {
            detail::StaticRuntimeMetadata const metadata_copy{metadata};
            return _log_statement_noinline<Args...>(macro_metadata, 0, &metadata_copy,
                                                    static_cast<Args&&>(fmt_args)...);
          }
        }

        detail::ThreadContext* const thread_context = _thread_context;

        if (QUILL_UNLIKELY(thread_context == nullptr))
        {
          detail::StaticRuntimeMetadata const metadata_copy{metadata};
          return _log_statement_noinline<Args...>(macro_metadata, 0, &metadata_copy,
                                                  static_cast<Args&&>(fmt_args)...);
        }

        queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

        size_t const total_size = s_header_size + sizeof(detail::StaticRuntimeMetadata) +
          detail::compute_encoded_size_and_cache_string_lengths(
                                    thread_context->get_conditional_arg_size_cache(), fmt_args...);

        auto const reservation = queue.prepare_write_reserve_cached(total_size);

        if (QUILL_UNLIKELY(reservation.write_buffer == nullptr))
        {
          detail::StaticRuntimeMetadata const metadata_copy{metadata};
          return _log_statement_noinline<Args...>(macro_metadata, total_size, &metadata_copy,
                                                  static_cast<Args&&>(fmt_args)...);
        }

        if (total_size > detail::prefetched_record_min_size)
        {
          detail::prefetch_record_for_write(reservation.write_buffer, total_size);
        }

        std::byte* write_buffer = reservation.write_buffer + s_header_size;

        detail::encode_static_runtime_metadata(write_buffer, metadata);

        detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                       static_cast<decltype(fmt_args)&&>(fmt_args)...);

        QUILL_ASSERT_WITH_FMT(
          total_size == static_cast<size_t>(write_buffer - reservation.write_buffer),
          "Encoded bytes mismatch in log_statement_macro_free(): total_size=%zu, "
          "actual_encoded=%zu, fmt=\"%s\"",
          total_size, static_cast<size_t>(write_buffer - reservation.write_buffer), metadata.fmt);

        _encode_header<true>(reservation.write_buffer, macro_metadata,
                             reinterpret_cast<uintptr_t>(detail::decoder_ptr<Args...>));

        // Commit through the cached queue pointer, which also rounds the record up to its alignment
        reservation.bounded_queue->finish_and_commit_write(total_size);

        _flush_after_log_statement_if_needed(macro_metadata);
        return true;
      }
    }

    if (QUILL_UNLIKELY(_thread_context == nullptr))
    {
      _thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    detail::ThreadContext* const thread_context = _thread_context;
    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    uint32_t const fmt_size = detail::clamp_encoded_string_length(detail::safe_strnlen(metadata.fmt) + 1u);
    uint32_t const tags_size = detail::clamp_encoded_string_length(detail::safe_strnlen(tags) + 1u);

    size_t const total_size = s_header_size + fmt_size + (sizeof(void const*) * 2u) + tags_size +
      sizeof(line) + sizeof(log_level) +
      detail::compute_encoded_size_and_cache_string_lengths(
                                thread_context->get_conditional_arg_size_cache(), fmt_args...);

    std::byte* const write_begin =
      _reserve_queue_space(queue, total_size, &s_hybrid_copy_metadata, thread_context);

    if (QUILL_UNLIKELY(write_begin == nullptr))
    {
      return false;
    }

    if (total_size > detail::prefetched_record_min_size)
    {
      detail::prefetch_record_for_write(write_begin, total_size);
    }

    std::byte* write_buffer = write_begin + s_header_size;

    detail::encode_hybrid_copy_metadata(write_buffer, metadata.fmt, fmt_size, metadata.file,
                                        metadata.function, tags, tags_size, line, log_level);

    detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                   static_cast<decltype(fmt_args)&&>(fmt_args)...);

    QUILL_ASSERT_WITH_FMT(total_size == static_cast<size_t>(write_buffer - write_begin),
                          "Encoded bytes mismatch in log_statement_macro_free(): total_size=%zu, "
                          "actual_encoded=%zu, fmt=\"%s\"",
                          total_size, static_cast<size_t>(write_buffer - write_begin), metadata.fmt);

    _encode_header(write_begin, &s_hybrid_copy_metadata,
                   reinterpret_cast<uintptr_t>(detail::decoder_ptr<Args...>));

    queue.finish_and_commit_write(total_size);
    _flush_after_log_statement_if_needed();

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
   * @return true if the message is written to the queue, false if it is dropped
   */
  template <typename... Args>
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

    if (QUILL_UNLIKELY(_thread_context == nullptr))
    {
      _thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    detail::ThreadContext* const thread_context = _thread_context;
    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    size_t total_size{s_header_size};
    uint32_t hybrid_fmt_len{0};
    uint32_t hybrid_tags_len{0};
    MacroMetadata::Event const event = macro_metadata->event();

    if (event == MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy)
    {
      // The hybrid path owns only fmt and tags. Cache their lengths in locals so the generic
      // argument-size cache is reserved for the format arguments and does not need four metadata
      // dispatches on every call.
      hybrid_fmt_len = detail::clamp_encoded_string_length(detail::safe_strnlen(fmt) + 1u);
      hybrid_tags_len = detail::clamp_encoded_string_length(detail::safe_strnlen(tags) + 1u);
      total_size += hybrid_fmt_len + (sizeof(void const*) * 2u) + hybrid_tags_len +
        sizeof(line_number) + sizeof(log_level);
      total_size += detail::compute_encoded_size_and_cache_string_lengths(
        thread_context->get_conditional_arg_size_cache(), fmt_args...);
    }
    else if (event == MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy)
    {
      total_size += detail::compute_encoded_size_and_cache_string_lengths(
        thread_context->get_conditional_arg_size_cache(), fmt, file_path, function_name, tags,
        line_number, log_level, fmt_args...);
    }
    else if (event == MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy)
    {
      total_size += detail::compute_encoded_size_and_cache_string_lengths(
        thread_context->get_conditional_arg_size_cache(), static_cast<void const*>(fmt),
        static_cast<void const*>(file_path), static_cast<void const*>(function_name),
        static_cast<void const*>(tags), line_number, log_level, fmt_args...);
    }
    else
    {
      return false;
    }

    std::byte* const write_begin = _reserve_queue_space(queue, total_size, macro_metadata, thread_context);

    if (QUILL_UNLIKELY(write_begin == nullptr))
    {
      return false;
    }

    if (total_size > detail::prefetched_record_min_size)
    {
      detail::prefetch_record_for_write(write_begin, total_size);
    }

    std::byte* write_buffer = write_begin + s_header_size;

    if (event == MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy)
    {
      detail::encode_hybrid_copy_metadata(write_buffer, fmt, hybrid_fmt_len, file_path, function_name,
                                          tags, hybrid_tags_len, line_number, log_level);
      detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                     static_cast<decltype(fmt_args)&&>(fmt_args)...);
    }
    else if (event == MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy)
    {
      detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(), fmt, file_path, function_name,
                     tags, line_number, log_level, static_cast<decltype(fmt_args)&&>(fmt_args)...);
    }
    else
    {
      detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                     static_cast<void const*>(fmt), static_cast<void const*>(file_path),
                     static_cast<void const*>(function_name), static_cast<void const*>(tags),
                     line_number, log_level, static_cast<decltype(fmt_args)&&>(fmt_args)...);
    }

    QUILL_ASSERT_WITH_FMT(total_size == static_cast<size_t>(write_buffer - write_begin),
                          "Encoded bytes mismatch in log_statement_runtime_metadata(): "
                          "total_size=%zu, actual_encoded=%zu, fmt=\"%s\"",
                          total_size, static_cast<size_t>(write_buffer - write_begin), fmt);

    _encode_header(write_begin, macro_metadata, reinterpret_cast<uintptr_t>(detail::decoder_ptr<Args...>));

    queue.finish_and_commit_write(total_size);
    _flush_after_log_statement_if_needed();

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

    // Pass the capacity as the event argument
    if (!_enqueue_control_event(&macro_metadata, max_capacity))
    {
      return;
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

    _enqueue_control_event(&macro_metadata);
  }

  /**
   * Blocks the calling thread until all log messages up to the current timestamp are flushed.
   *
   * The backend thread will invoke the write operation on all sinks for all loggers up to the point
   * (timestamp) when this function is invoked.
   *
   * @param sleep_duration_ns The requested sleep in nanoseconds between retries when the
   * blocking queue is full and between checks for flush completion. Default is 100 nanoseconds.
   * The initial checks yield. On Windows a sleep is rounded up to whole milliseconds.
   * 0 always yields.
   *
   * @warning Do not call this function from the destructor of a static object. This may lead to application crashes if the thread-local ThreadContext is destroyed before the static object invoking `flush_log`.
   * @note This function should only be called when the backend worker is running after Backend::start(...)
   * @note This function will block the calling thread until the flush message is processed by the backend thread.
   *       The calling thread can block for up to backend_options.sleep_duration. If you configure a custom
   *       long sleep duration on the backend thread, e.g., backend_options.sleep_duration = std::chrono::minutes{1},
   *       then you should ideally avoid calling this function as you can block for long period of times unless
   *       you use another thread that calls Backend::notify()
   */
  // Not inlined: every log statement reaches this through the immediate-flush check, and inlining
  // the wait loops there costs each call site a stack frame and saved registers on its hot path.
  QUILL_NOINLINE void flush_log(uint32_t sleep_duration_ns = 100)
  {
    if (QUILL_UNLIKELY(detail::LoggerBase::is_current_thread_backend_thread()))
    {
      // flush_log() would wait on the backend worker to process the flush event,
      // but we are the backend worker — surface the misuse instead of self-deadlocking.
      QUILL_THROW(
        QuillError{"logger->flush_log() cannot be called from the backend worker thread"});
    }

    // The same check as log_statement()
    QUILL_ASSERT(_valid.load(std::memory_order_acquire),
                 "Attempting to log with an invalidated logger");

    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::Flush};

    std::atomic<bool> backend_thread_flushed{false};
    std::atomic<bool>* backend_thread_flushed_ptr = &backend_thread_flushed;

    // We do not want to drop the message if a dropping queue is used
    uint32_t attempt{0};
    while (!_enqueue_flush_event(&macro_metadata, reinterpret_cast<uintptr_t>(backend_thread_flushed_ptr)))
    {
      if constexpr (using_unbounded_queue)
      {
        if (_thread_context->last_event_larger_than_queue())
        {
          QUILL_THROW(QuillError{"Could not grow the queue to enqueue the flush request"});
        }
      }

      detail::backoff_wait(attempt, sleep_duration_ns);
    }

    // The caller thread keeps checking the flag until the backend thread flushes
    attempt = 0;
    while (!backend_thread_flushed.load())
    {
      detail::backoff_wait(attempt, sleep_duration_ns);
    }
  }

private:
  friend class detail::LoggerManager;
  friend class detail::BackendWorker;

  /**
   * Enqueues a control event that must not be dropped, retrying while a dropping queue is full.
   * The backend thread cannot wait for itself to drain the queue, so there a full queue returns
   * false without enqueuing. So does an event larger than an unbounded queue that could not
   * grow, which is dropped and counted because retrying could never enqueue it.
   */
  template <typename... Args>
  bool _enqueue_control_event(MacroMetadata const* macro_metadata, Args&&... args)
  {
    while (!this->log_statement(macro_metadata, static_cast<Args&&>(args)...))
    {
      if (QUILL_UNLIKELY(detail::LoggerBase::is_current_thread_backend_thread()))
      {
        return false;
      }

      if (QUILL_UNLIKELY(_thread_context->last_event_larger_than_queue()))
      {
        return false;
      }

      detail::sleep_for_ns(100);
    }

    return true;
  }

  /**
   * Writes the record of flush_log(), the address of the flag that the backend sets. The backend
   * reads the address itself and calls no decoder, so this does not instantiate a
   * log_statement<uintptr_t> in every file that logs, where flush_log() is compiled
   */
  QUILL_NODISCARD bool _enqueue_flush_event(MacroMetadata const* macro_metadata, uintptr_t flush_flag)
  {
    if (QUILL_UNLIKELY(_thread_context == nullptr))
    {
      _thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    detail::ThreadContext* const thread_context = _thread_context;
    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    constexpr size_t total_size = s_header_size + sizeof(flush_flag);

    std::byte* const write_begin = _reserve_queue_space(queue, total_size, macro_metadata, thread_context);

    if (QUILL_UNLIKELY(write_begin == nullptr))
    {
      return false;
    }

    std::memcpy(write_begin + s_header_size, &flush_flag, sizeof(flush_flag));
    _encode_header(write_begin, macro_metadata, 0);

    queue.finish_and_commit_write(total_size);

    return true;
  }

  static_assert(sizeof(detail::FormatArgsDecoder) == sizeof(uintptr_t),
                "FormatArgsDecoder must fit in uintptr_t for header encoding");
  static_assert(sizeof(uintptr_t) <= sizeof(uint64_t),
                "Header encoding requires pointers to fit in 64 bits");
  static_assert(sizeof(double) == sizeof(uint64_t),
                "publish_metric stores a double in the uint64_t decoder slot of the header");

  static constexpr size_t s_header_size = 4 * sizeof(uint64_t);

  /**
   * The metadata of the records of log_statement_macro_free() that refer to a literal format
   * string, by log level, which these records do not hold
   */
  static constexpr MacroMetadata s_static_runtime_metadata[LogLevelCount]{
    MacroMetadata{"", "", "", nullptr, LogLevel::TraceL3, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::TraceL2, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::TraceL1, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::Debug, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::Info, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::Notice, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::Warning, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::Error, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::Backtrace, MacroMetadata::Event::LogWithStaticRuntimeMetadata},
    MacroMetadata{"", "", "", nullptr, LogLevel::None, MacroMetadata::Event::LogWithStaticRuntimeMetadata}};

  /** The metadata of the records of log_statement_macro_free() that copy the format string */
  static constexpr MacroMetadata s_hybrid_copy_metadata{
    "", "", "", nullptr, LogLevel::None, MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy};

  static_assert(max_binary_record_size <= (SIZE_MAX - s_header_size - sizeof(uint32_t)),
                "The queue space of the largest binary record must not overflow size_t");

  /** Not inlined: it keeps the message of the error out of the callers of log_binary() */
  QUILL_NOINLINE QUILL_ATTRIBUTE_COLD static void _throw_binary_record_too_large()
  {
    QUILL_THROW(QuillError{"A binary record is larger than the maximum of " +
                           std::to_string(max_binary_record_size) + " bytes"});
  }

  /***/
  LoggerImpl(std::string logger_name, std::vector<std::shared_ptr<Sink>> sinks,
             PatternFormatterOptions pattern_formatter_options, ClockSourceType clock_source,
             clock_object_t* clock)
    : detail::LoggerBase(static_cast<std::string&&>(logger_name),
                         static_cast<std::vector<std::shared_ptr<Sink>>&&>(sinks),
                         static_cast<PatternFormatterOptions&&>(pattern_formatter_options),
                         clock ? ClockSourceType::User : clock_source, clock)
  {
    if constexpr (!using_runtime_clock)
    {
      if (!clock)
      {
        QUILL_THROW(QuillError{"FrontendOptions::clock_type requires a clock object"});
      }
    }
  }

  /**
   * Reserves space for a metric when the fast path cannot commit it directly.
   */
  QUILL_NODISCARD QUILL_NOINLINE QUILL_NOCLONE bool _publish_metric_noinline(MetricMetadata const* metric_metadata,
                                                                             double value)
  {
    if (QUILL_UNLIKELY(_thread_context == nullptr))
    {
      _thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    detail::ThreadContext* const thread_context = _thread_context;
    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    constexpr size_t total_size = s_header_size;

    std::byte* const write_begin = _reserve_queue_space(queue, total_size, metric_metadata, thread_context);

    if (QUILL_UNLIKELY(write_begin == nullptr))
    {
      return false;
    }

    uint64_t value_bits;
    std::memcpy(&value_bits, &value, sizeof(value_bits));

    _encode_header(write_begin, metric_metadata, value_bits);

    queue.finish_and_commit_write(total_size);

    return true;
  }

  /**
   * Slow path for log_statement() and log_statement_macro_free(). Small, safely copyable arguments
   * are accepted by value so the hot caller can keep them in registers; all other arguments retain
   * their original reference type.
   * @param total_size the encoded size computed by the caller, or zero to compute it here.
   * Computing it twice would run a DirectFormatCodec formatter twice for one log call.
   * @param static_runtime_metadata the metadata that log_statement_macro_free() writes after the
   * header, nullptr for log_statement()
   */
  template <typename... OriginalArgs>
  QUILL_NODISCARD QUILL_NOINLINE QUILL_NOCLONE bool _log_statement_noinline(
    MacroMetadata const* macro_metadata, size_t total_size, detail::StaticRuntimeMetadata const* static_runtime_metadata,
    typename detail::slow_path_arg<OriginalArgs>::type... fmt_args)
  {
    if (QUILL_UNLIKELY(_thread_context == nullptr))
    {
      _thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    detail::ThreadContext* const thread_context = _thread_context;
    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    if (total_size == 0)
    {
      size_t const metadata_size = static_runtime_metadata ? sizeof(detail::StaticRuntimeMetadata) : 0u;
      total_size = s_header_size + metadata_size +
        detail::compute_encoded_size_and_cache_string_lengths(
                     thread_context->get_conditional_arg_size_cache(), fmt_args...);
    }

    std::byte* const write_begin = _reserve_queue_space(queue, total_size, macro_metadata, thread_context);

    if (QUILL_UNLIKELY(write_begin == nullptr))
    {
      return false;
    }

    if (total_size > detail::prefetched_record_min_size)
    {
      // Loggers with a system or runtime user clock always take this path
      detail::prefetch_record_for_write(write_begin, total_size);
    }

    std::byte* write_buffer = write_begin + s_header_size;

    if (static_runtime_metadata != nullptr)
    {
      detail::encode_static_runtime_metadata(write_buffer, *static_runtime_metadata);
    }

    detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(),
                   static_cast<OriginalArgs&&>(fmt_args)...);

    QUILL_ASSERT_WITH_FMT(
      total_size == static_cast<size_t>(write_buffer - write_begin),
      "Encoded bytes mismatch in _log_statement_noinline(): total_size=%zu, "
      "actual_encoded=%zu, msg=\"%s\"",
      total_size, static_cast<size_t>(write_buffer - write_begin),
      static_runtime_metadata ? static_runtime_metadata->fmt : macro_metadata->message_format());

    _encode_header(write_begin, macro_metadata,
                   reinterpret_cast<uintptr_t>(detail::decoder_ptr<OriginalArgs...>));

    queue.finish_and_commit_write(total_size);
    _flush_after_log_statement_if_needed(macro_metadata);

    return true;
  }

  QUILL_NODISCARD static std::string_view _label_key(MetricLabel const& label) noexcept
  {
    return label.key;
  }
  QUILL_NODISCARD static std::string_view _label_value(MetricLabel const& label) noexcept
  {
    return label.value;
  }
  QUILL_NODISCARD static std::string_view _label_key(std::pair<std::string_view, std::string_view> const& label) noexcept
  {
    return label.first;
  }
  QUILL_NODISCARD static std::string_view _label_value(std::pair<std::string_view, std::string_view> const& label) noexcept
  {
    return label.second;
  }

  /** Encodes the metric name, label count and each label's key and value after the header. */
  template <typename TLabels>
  bool _publish_dynamic_metric(std::string_view metric_name, TLabels const& labels, double value)
  {
    QUILL_ASSERT(_valid.load(std::memory_order_acquire),
                 "Attempting to log with an invalidated logger");

    if (QUILL_UNLIKELY(metric_name.empty()))
    {
      QUILL_THROW(QuillError{"Metric name must not be empty"});
    }

    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::None, MacroMetadata::Event::DynamicMetric};

    if (QUILL_UNLIKELY(_thread_context == nullptr))
    {
      _thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    detail::ThreadContext* const thread_context = _thread_context;
    auto& size_cache = thread_context->get_conditional_arg_size_cache();

    size_t const label_count = labels.size();
    size_t total_size = s_header_size;
    total_size += detail::compute_encoded_size_and_cache_string_lengths(size_cache, metric_name, label_count);

    for (auto const& label : labels)
    {
      total_size += detail::compute_encoded_size_and_cache_string_lengths(
        size_cache, _label_key(label), _label_value(label));
    }

    queue_t& queue = thread_context->get_spsc_queue<frontend_options_t::queue_type>();

    std::byte* const write_begin = _reserve_queue_space(queue, total_size, &macro_metadata, thread_context);

    if (QUILL_UNLIKELY(write_begin == nullptr))
    {
      return false;
    }

    std::byte* write_buffer = write_begin + s_header_size;

    detail::encode(write_buffer, size_cache, metric_name, label_count);

    for (auto const& label : labels)
    {
      detail::encode(write_buffer, size_cache, _label_key(label), _label_value(label));
    }

    uint64_t value_bits;
    std::memcpy(&value_bits, &value, sizeof(value_bits));

    _encode_header(write_begin, &macro_metadata, value_bits);

    queue.finish_and_commit_write(total_size);
    return true;
  }

  /** The inlined log and metric paths have already dispatched runtime clocks other than TSC. */
  template <bool RuntimeClockIsTsc>
  QUILL_NODISCARD uint64_t _get_timestamp() const noexcept
  {
    if constexpr (using_runtime_clock)
    {
      if constexpr (!RuntimeClockIsTsc)
      {
        if (QUILL_UNLIKELY(_clock_source != ClockSourceType::Tsc))
        {
          return _user_clock ? _user_clock->now() : detail::get_system_time_ns();
        }
      }
      return detail::rdtsc();
    }
    else
    {
      return static_cast<clock_type const*>(_user_clock)->clock_type::now();
    }
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
    std::byte* const write_buffer = queue.prepare_write(total_size);

    if (write_buffer != nullptr)
    {
      return write_buffer;
    }

    return _reserve_queue_space_when_full(queue, total_size, macro_metadata, thread_context);
  }

  /**
   * Blocks or drops, as the queue type says, when the queue has no space for a record. Not inlined:
   * it does not depend on the arguments, so the slow path of every signature calls this one copy
   */
  QUILL_NODISCARD QUILL_NOINLINE QUILL_ATTRIBUTE_COLD std::byte* _reserve_queue_space_when_full(
    queue_t& queue, size_t total_size, MacroMetadata const* macro_metadata, detail::ThreadContext* thread_context)
  {
    std::byte* write_buffer{nullptr};

    if constexpr ((frontend_options_t::queue_type == QueueType::BoundedDropping) ||
                  (frontend_options_t::queue_type == QueueType::UnboundedDropping))
    {
      if constexpr (frontend_options_t::queue_type == QueueType::UnboundedDropping)
      {
        if (QUILL_UNLIKELY(thread_context->drop_event_if_larger_than_queue(total_size, queue.producer_capacity())))
        {
          // The queue could not grow and the event does not fit in it even when empty. This
          // includes control events, whose callers would otherwise retry forever.
          return nullptr;
        }
      }

      // Not enough space to push: bump the shared failure counter for log, binary and metric
      // drops so that none silently disappears. They reuse the existing counter rather than
      // adding parallel ones; the error-notifier message reflects the combined count.
      // Internal control events (flush, mdc updates, backtrace control, logger removal) are
      // not counted: their callers retry in a loop until the event is enqueued, so a failed
      // attempt would report a phantom drop for an event that is eventually delivered
      MacroMetadata::Event const event = macro_metadata->event();

      if ((event == MacroMetadata::Event::Log) || (event == MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy) ||
          (event == MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy) ||
          (event == MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy) ||
          (event == MacroMetadata::Event::LogWithStaticRuntimeMetadata) ||
          (event == MacroMetadata::Event::Metric) ||
          (event == MacroMetadata::Event::DynamicMetric) || (event == MacroMetadata::Event::Binary))
      {
        thread_context->increment_failure_counter();
      }
      else if constexpr (frontend_options_t::queue_type == QueueType::BoundedDropping)
      {
        if (QUILL_UNLIKELY(total_size > queue.capacity()))
        {
          QUILL_THROW(
            QuillError{"Control event size exceeds the configured bounded queue capacity: " +
                       std::to_string(total_size) + " > " + std::to_string(queue.capacity())});
        }
      }
    }
    else if constexpr (frontend_options_t::queue_type == QueueType::BoundedBlocking)
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

      if (QUILL_UNLIKELY(detail::LoggerBase::is_current_thread_backend_thread()))
      {
        // The backend is the only consumer of this queue, so waiting here would self-deadlock.
        // Do not increment the failure counter: reporting this drop through error_notifier could
        // recursively fill the same queue.
        return nullptr;
      }

      // Both log and metric events bump the counter; blocked events are reported as blocking
      // occurrences, so unlike the dropping-queue branch no control-event exclusion is needed.
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
    else if constexpr (frontend_options_t::queue_type == QueueType::UnboundedBlocking)
    {
      if (QUILL_UNLIKELY(detail::LoggerBase::is_current_thread_backend_thread()))
      {
        // The backend is the only consumer of this queue, so waiting here would self-deadlock.
        // Do not increment the failure counter: reporting this drop through error_notifier could
        // recursively fill the same queue.
        return nullptr;
      }

      if (QUILL_UNLIKELY(thread_context->drop_event_if_larger_than_queue(total_size, queue.producer_capacity())))
      {
        // The queue could not grow and the event does not fit in it even when empty, so waiting
        // for the backend to drain it would never end.
        return nullptr;
      }

      // Both log and metric events bump the counter; blocked events are reported as blocking
      // occurrences, so unlike the dropping-queue branch no control-event exclusion is needed.
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

    return write_buffer;
  }

  /**
   * Applies the per-logger immediate-flush policy after a log statement has been committed.
   *
   * Backend callbacks are allowed to log, but the backend thread cannot wait for itself to process
   * a flush event. In that case the message remains queued and is processed after the callback
   * returns to the backend loop.
   */
  QUILL_ATTRIBUTE_HOT inline void _flush_after_log_statement_if_needed(MacroMetadata const* macro_metadata = nullptr)
  {
    if constexpr (!frontend_options_t::enable_immediate_flush)
    {
      return;
    }

    uint32_t const threshold = _message_flush_threshold.load(std::memory_order_relaxed);
    if (QUILL_LIKELY(threshold == 0))
    {
      return;
    }

    // Runtime-metadata calls always log and omit metadata here. Other callers supply metadata
    // so control events cannot trigger recursive flushing.
    if (macro_metadata && (macro_metadata->event() != MacroMetadata::Event::Log) &&
        (macro_metadata->event() != MacroMetadata::Event::LogWithStaticRuntimeMetadata))
    {
      return;
    }

    _flush_after_log_statement(threshold);
  }

  /**
   * Counts the log statement and flushes when the threshold is reached.
   *
   * It only runs after set_immediate_flush(), a debugging aid, so it is cold and out of line. The
   * checks above are inlined into every log statement, and inlining this code with them made every
   * call site larger and slower to compile, e.g. 83 of the 321 bytes of a LOG_INFO with two ints on
   * MSVC /O2.
   */
  QUILL_NOINLINE QUILL_ATTRIBUTE_COLD void _flush_after_log_statement(uint32_t threshold)
  {
    if (threshold == 1)
    {
      if (QUILL_LIKELY(!detail::LoggerBase::is_current_thread_backend_thread()))
      {
        this->flush_log();
      }
      return;
    }

    uint32_t message_count = _messages_since_last_flush.fetch_add(1, std::memory_order_relaxed) + 1;
    while (message_count >= threshold)
    {
      // Only the thread that successfully resets the counter is allowed to flush.
      if (_messages_since_last_flush.compare_exchange_weak(
            message_count, 0, std::memory_order_relaxed, std::memory_order_relaxed))
      {
        if (QUILL_LIKELY(!detail::LoggerBase::is_current_thread_backend_thread()))
        {
          this->flush_log();
        }
        return;
      }
    }
  }

  /**
   * Writes the four header words after the arguments, just before the queue commit. Store the
   * decoder (or metric value) before reading the clock so it need not survive a clock call.
   */
  template <bool RuntimeClockIsTsc = false>
  QUILL_ATTRIBUTE_HOT void _encode_header(std::byte* record, MacroMetadata const* macro_metadata,
                                          uint64_t decoder) const noexcept
  {
    uint64_t const metadata = reinterpret_cast<uintptr_t>(macro_metadata);
    uint64_t const logger = reinterpret_cast<uintptr_t>(this);
    std::memcpy(record + (3 * sizeof(uint64_t)), &decoder, sizeof(decoder));

    uint64_t const timestamp = _get_timestamp<RuntimeClockIsTsc>();
    std::memcpy(record, &timestamp, sizeof(timestamp));
    std::memcpy(record + sizeof(uint64_t), &metadata, sizeof(metadata));
    std::memcpy(record + (2 * sizeof(uint64_t)), &logger, sizeof(logger));
  }
};

using Logger = LoggerImpl<FrontendOptions>;

QUILL_END_EXPORT

#if defined(_WIN32) && defined(_MSC_VER)
  #pragma warning(pop)
#endif

QUILL_END_NAMESPACE
