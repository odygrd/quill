/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/UserClockSource.h"
#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/Common.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/LogLevel.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/Rdtsc.h"

#include <atomic>
#include <cassert>
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
class Sink;

namespace detail
{
class LoggerManager;
class BackendWorker;
} // namespace detail

/**
 * @brief Thread safe logger.
 *
 * Logger must be obtained from create_or_get_logger(), therefore constructors are private
 */
template <typename TFrontendOptions>
class LoggerImpl : public detail::LoggerBase
{
public:
  using frontend_options_t = TFrontendOptions;

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
   * @param dynamic_log_level dynamic log level
   * @param macro_metadata metadata of the log message
   * @param fmt_args arguments
   *
   * @return true if the message is written to the queue, false if it is dropped (when a dropping queue is used)
   */
  template <bool immediate_flush, typename... Args>
  QUILL_ATTRIBUTE_HOT bool log_statement(LogLevel dynamic_log_level,
                                         MacroMetadata const* macro_metadata, Args&&... fmt_args)
  {
#ifndef NDEBUG
    if (dynamic_log_level != quill::LogLevel::None)
    {
      assert((macro_metadata->log_level() == quill::LogLevel::Dynamic) &&
             "MacroMetadata LogLevel must be Dynamic when using a dynamic_log_level");
    }

    if (macro_metadata->log_level() != quill::LogLevel::Dynamic)
    {
      assert((dynamic_log_level == quill::LogLevel::None) &&
             "No dynamic_log_level should be set when MacroMetadata LogLevel is not Dynamic");
    }

    assert(valid.load(std::memory_order_acquire) && "Invalidated loggers can not log");
#endif

    // Store the timestamp of the log statement at the start of the call. This gives more accurate
    // timestamp especially if the queue is full
    // This is very rare but might lead to out of order timestamp in the log file if we block on push for too long
    uint64_t const current_timestamp = (clock_source == ClockSourceType::Tsc) ? detail::rdtsc()
      : (clock_source == ClockSourceType::System)
      ? static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count())
      : user_clock->now();

    if (QUILL_UNLIKELY(thread_context == nullptr))
    {
      // This caches the ThreadContext pointer to avoid repeatedly calling get_local_thread_context()
      thread_context = detail::get_local_thread_context<frontend_options_t>();
    }

    // Need to know how much size we need from the queue
    size_t total_size = sizeof(current_timestamp) + (sizeof(uintptr_t) * 3) +
      detail::compute_encoded_size_and_cache_string_lengths(
                          thread_context->get_conditional_arg_size_cache(), fmt_args...);

    if (dynamic_log_level != LogLevel::None)
    {
      // For the dynamic log level we want to add to the total size to store the dynamic log level
      total_size += sizeof(dynamic_log_level);
    }

    constexpr bool is_unbounded_queue = (frontend_options_t::queue_type == QueueType::UnboundedUnlimited) ||
      (frontend_options_t::queue_type == QueueType::UnboundedBlocking) ||
      (frontend_options_t::queue_type == QueueType::UnboundedDropping);

    std::byte* write_buffer;

    if constexpr (is_unbounded_queue)
    {
      write_buffer = thread_context->get_spsc_queue<frontend_options_t::queue_type>().prepare_write(
        static_cast<uint32_t>(total_size), frontend_options_t::queue_type);
    }
    else
    {
      write_buffer = thread_context->get_spsc_queue<frontend_options_t::queue_type>().prepare_write(
        static_cast<uint32_t>(total_size));
    }

    if constexpr (frontend_options_t::queue_type == QueueType::UnboundedUnlimited)
    {
      assert(write_buffer &&
             "Unbounded unlimited queue will always allocate and have enough capacity");
    }
    else if constexpr ((frontend_options_t::queue_type == QueueType::BoundedDropping) ||
                       (frontend_options_t::queue_type == QueueType::UnboundedDropping))
    {
      if (QUILL_UNLIKELY(write_buffer == nullptr))
      {
        // not enough space to push to queue message is dropped
        thread_context->increment_failure_counter();
        return false;
      }
    }
    else if constexpr ((frontend_options_t::queue_type == QueueType::BoundedBlocking) ||
                       (frontend_options_t::queue_type == QueueType::UnboundedBlocking))
    {
      if (QUILL_UNLIKELY(write_buffer == nullptr))
      {
        thread_context->increment_failure_counter();

        do
        {
          if constexpr (frontend_options_t::blocking_queue_retry_interval_ns > 0)
          {
            std::this_thread::sleep_for(std::chrono::nanoseconds{frontend_options_t::blocking_queue_retry_interval_ns});
          }

          // not enough space to push to queue, keep trying
          if constexpr (is_unbounded_queue)
          {
            write_buffer = thread_context->get_spsc_queue<frontend_options_t::queue_type>().prepare_write(
              static_cast<uint32_t>(total_size), frontend_options_t::queue_type);
          }
          else
          {
            write_buffer = thread_context->get_spsc_queue<frontend_options_t::queue_type>().prepare_write(
              static_cast<uint32_t>(total_size));
          }
        } while (write_buffer == nullptr);
      }
    }

    // we have enough space in this buffer, and we will write to the buffer

#ifndef NDEBUG
    std::byte const* const write_begin = write_buffer;
    assert(write_begin);
#endif

    // first encode a header
    write_buffer = _encode_header(write_buffer, current_timestamp, macro_metadata, this,
                                  detail::decode_and_store_args<detail::remove_cvref_t<Args>...>);

    // encode remaining arguments
    detail::encode(write_buffer, thread_context->get_conditional_arg_size_cache(), fmt_args...);

    if (dynamic_log_level != LogLevel::None)
    {
      // write the dynamic log level
      // The reason we write it last is that is less likely to break the alignment in the buffer
      std::memcpy(write_buffer, &dynamic_log_level, sizeof(dynamic_log_level));
      write_buffer += sizeof(dynamic_log_level);
    }

#ifndef NDEBUG
    assert((write_buffer > write_begin) && "write_buffer must be greater than write_begin");
    assert(total_size == (static_cast<uint32_t>(write_buffer - write_begin)) &&
           "The committed write bytes must be equal to the total_size requested bytes");
#endif

    thread_context->get_spsc_queue<frontend_options_t::queue_type>().finish_write(static_cast<uint32_t>(total_size));
    thread_context->get_spsc_queue<frontend_options_t::queue_type>().commit_write();
    thread_context->get_conditional_arg_size_cache().clear();

    if constexpr (immediate_flush)
    {
      this->flush_log();
    }

    return true;
  }

  /**
   * Init a backtrace for this logger.
   * Stores messages logged with LOG_BACKTRACE in a ring buffer messages and displays them later on demand.
   * @param max_capacity The max number of messages to store in the backtrace
   * @param flush_level If this loggers logs any message higher or equal to this severity level the backtrace will also get flushed.
   * Default level is None meaning the user has to call flush_backtrace explicitly
   */
  void init_backtrace(uint32_t max_capacity, LogLevel flush_level = LogLevel::None)
  {
    // we do not care about the other fields, except quill::MacroMetadata::Event::InitBacktrace
    static constexpr MacroMetadata macro_metadata{
      "", "", "{}", nullptr, LogLevel::Critical, MacroMetadata::Event::InitBacktrace};

    // we pass this message to the queue and also pass capacity as arg
    // We do not want to drop the message if a dropping queue is used
    while (!this->log_statement<false>(LogLevel::None, &macro_metadata, max_capacity))
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    }

    // Also store the desired flush log level
    backtrace_flush_level.store(flush_level, std::memory_order_relaxed);
  }

  /**
   * Dump any stored backtrace messages
   */
  void flush_backtrace()
  {
    // we do not care about the other fields, except quill::MacroMetadata::Event::Flush
    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::FlushBacktrace};

    // We do not want to drop the message if a dropping queue is used
    while (!this->log_statement<false>(LogLevel::None, &macro_metadata))
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
    while (!this->log_statement<false>(LogLevel::None, &macro_metadata,
                                       reinterpret_cast<uintptr_t>(backend_thread_flushed_ptr)))
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
  LoggerImpl(std::string logger_name, std::vector<std::shared_ptr<Sink>> sinks,
             PatternFormatterOptions pattern_formatter_options, ClockSourceType clock_source,
             UserClockSource* user_clock)
    : detail::LoggerBase(static_cast<std::string&&>(logger_name),
                         static_cast<std::vector<std::shared_ptr<Sink>>&&>(sinks),
        static_cast<PatternFormatterOptions&&>(pattern_formatter_options), clock_source, user_clock)

  {
    if (this->user_clock)
    {
      // if a user clock is provided then set the ClockSourceType to User
      this->clock_source = ClockSourceType::User;
    }
  }
  
  /**
   * Encodes header information into the write buffer
   *
   * This function helps the compiler to better optimize the operation using SSE/AVX instructions.
   * Avoid using `std::byte*& write_buffer` as parameter as it may prevent these optimizations.
   *
   * @return Updated pointer to the write buffer after encoding the header.
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT static std::byte* _encode_header(std::byte* write_buffer, uint64_t timestamp,
                                                                       MacroMetadata const* metadata,
                                                                       detail::LoggerBase* logger_ctx,
                                                                       detail::FormatArgsDecoder decoder) noexcept
  {
    std::memcpy(write_buffer, &timestamp, sizeof(timestamp));
    write_buffer += sizeof(timestamp);

    std::memcpy(write_buffer, &metadata, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    std::memcpy(write_buffer, &logger_ctx, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    std::memcpy(write_buffer, &decoder, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    return write_buffer;
  }
};

using Logger = LoggerImpl<FrontendOptions>;

QUILL_END_NAMESPACE
