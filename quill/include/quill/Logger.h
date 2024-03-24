/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/UserClockSource.h"
#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/EncodeDecode.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/LogLevel.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/Rdtsc.h"
#include "quill/core/ThreadContextManager.h"

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

namespace quill
{

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
   * @param metadata metadata of the log message
   * @param format_args arguments
   *
   * @return true if the message is written to the queue, false if it is dropped (when a dropping queue is used)
   */
  template <typename... Args>
  QUILL_ALWAYS_INLINE_HOT bool log_message(LogLevel dynamic_log_level,
                                           MacroMetadata const* macro_metadata, Args&&... fmt_args)
  {
    assert(valid.load(std::memory_order_acquire) && "Invalidated loggers can not log");

    // Store the timestamp of the log statement at the start of the call. This gives more accurate
    // timestamp especially if the queue is full
    uint64_t const current_timestamp = (clock_source == ClockSourceType::Tsc) ? detail::rdtsc()
      : (clock_source == ClockSourceType::System)
      ? static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count())
      : user_clock->now();

    detail::ThreadContext* const thread_context =
      quill::detail::get_local_thread_context<frontend_options_t>();

    // For windows also take wide strings into consideration.
#if defined(_WIN32)
    constexpr uint32_t c_string_count = detail::count_c_style_strings<Args...>() +
      detail::count_c_style_wide_strings<Args...>() + detail::count_std_wstring_type<Args...>();
#else
    constexpr uint32_t c_string_count = detail::count_c_style_strings<Args...>();
#endif

    size_t c_string_sizes[(c_string_count > 1 ? c_string_count : 1)];

    // Need to reserve additional space as we will be aligning the pointer
    size_t total_size = sizeof(current_timestamp) + (sizeof(uintptr_t) * 3) +
      detail::calculate_args_size_and_populate_string_lengths(c_string_sizes, fmt_args...);

    if (dynamic_log_level != LogLevel::None)
    {
      // For the dynamic log level we want to add to the total size to store the dynamic log level
      total_size += sizeof(dynamic_log_level);
    }

    // request this size from the queue
    std::byte* write_buffer;
    if constexpr ((frontend_options_t::queue_type == QueueType::UnboundedUnlimited) ||
                  (frontend_options_t::queue_type == QueueType::UnboundedBlocking) ||
                  (frontend_options_t::queue_type == QueueType::UnboundedBlocking))
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
          if constexpr ((frontend_options_t::queue_type == QueueType::UnboundedUnlimited) ||
                        (frontend_options_t::queue_type == QueueType::UnboundedBlocking) ||
                        (frontend_options_t::queue_type == QueueType::UnboundedBlocking))
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

    // Then write the pointer to the LogDataNode. The LogDataNode has all details on how to
    // deserialize the object. We will just serialize the arguments in our queue, but we need
    // to look up their types to deserialize them

#ifndef NDEBUG
    std::byte const* const write_begin = write_buffer;
    assert(write_begin);
#endif

    std::memcpy(write_buffer, &current_timestamp, sizeof(current_timestamp));
    write_buffer += sizeof(current_timestamp);

    std::memcpy(write_buffer, &macro_metadata, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    detail::LoggerBase const* logger_context = this;
    std::memcpy(write_buffer, &logger_context, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    detail::FormatArgsDecoder const ftf =
      detail::decode_and_populate_format_args<detail::remove_cvref_t<Args>...>;
    std::memcpy(write_buffer, &ftf, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    // encode remaining arguments
    detail::encode(write_buffer, c_string_sizes, fmt_args...);

    if (dynamic_log_level != LogLevel::None)
    {
      // write the dynamic log level
      std::memcpy(write_buffer, &dynamic_log_level, sizeof(dynamic_log_level));
      write_buffer += sizeof(LogLevel);
    }

#ifndef NDEBUG
    assert((write_buffer > write_begin) && "write_buffer must be greater than write_begin");
    assert(total_size == (static_cast<uint32_t>(write_buffer - write_begin)) &&
           "The committed write bytes must be equal to the total_size requested bytes");
#endif

    thread_context->get_spsc_queue<frontend_options_t::queue_type>().finish_write(static_cast<uint32_t>(total_size));
    thread_context->get_spsc_queue<frontend_options_t::queue_type>().commit_write();

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
      "", "", "{}", nullptr, LogLevel::Critical, MacroMetadata::Event::InitBacktrace, false};

    // we pass this message to the queue and also pass capacity as arg
    // We do not want to drop the message if a dropping queue is used
    while (!this->log_message(LogLevel::None, &macro_metadata, max_capacity))
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
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::FlushBacktrace, false};

    // We do not want to drop the message if a dropping queue is used
    while (!this->log_message(LogLevel::None, &macro_metadata))
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
   * @note This function should only be called when the backend worker is running after Backend::start(...)
   * @note This function will block the calling thread until the flush message is processed by the backend thread.
   *       The calling thread can block for up to backend_options.sleep_duration. If you configure a custom
   *       high sleep duration on the backend thread, e.g., backend_options.sleep_duration = std::chrono::minutes{1},
   *       then you should call Backend::notify() prior to calling this function to ensure you are not blocked
   *       while the backend worker thread is sleeping.
   */
  void flush_log()
  {
    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::Flush, false};

    std::atomic<bool> backend_thread_flushed{false};
    std::atomic<bool>* backend_thread_flushed_ptr = &backend_thread_flushed;

    // We do not want to drop the message if a dropping queue is used
    while (!this->log_message(LogLevel::None, &macro_metadata, reinterpret_cast<uintptr_t>(backend_thread_flushed_ptr)))
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    }

    // The caller thread keeps checking the flag until the backend thread flushes
    do
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    } while (!backend_thread_flushed.load());
  }

private:
  friend class detail::LoggerManager;
  friend class detail::BackendWorker;

  /***/
  LoggerImpl(std::string logger_name, std::shared_ptr<Sink> sink, std::string format_pattern,
             std::string time_pattern, Timezone timestamp_timezone, ClockSourceType clock_source,
             UserClockSource* user_clock)
    : detail::LoggerBase(static_cast<std::string&&>(logger_name), static_cast<std::shared_ptr<Sink>&&>(sink),
                         static_cast<std::string&&>(format_pattern),
                         static_cast<std::string&&>(time_pattern), timestamp_timezone, clock_source, user_clock)
  {
    if (this->user_clock)
    {
      // if a user clock is provided then set the ClockSourceType to User
      this->clock_source = ClockSourceType::User;
    }
  }

  /***/
  LoggerImpl(std::string logger_name, std::vector<std::shared_ptr<Sink>> const& sinks,
             std::string format_pattern, std::string time_pattern, Timezone timestamp_timezone,
             ClockSourceType clock_source, UserClockSource* user_clock)
    : detail::LoggerBase(
        static_cast<std::string&&>(logger_name), sinks, static_cast<std::string&&>(format_pattern),
        static_cast<std::string&&>(time_pattern), timestamp_timezone, clock_source, user_clock)

  {
    if (this->user_clock)
    {
      // if a user clock is provided then set the ClockSourceType to User
      this->clock_source = ClockSourceType::User;
    }
  }
};

using Logger = LoggerImpl<FrontendOptions>;
} // namespace quill
