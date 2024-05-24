/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/BackendOptions.h"
#include "quill/backend/BackendUtilities.h"
#include "quill/backend/BacktraceStorage.h"
#include "quill/backend/PatternFormatter.h"
#include "quill/backend/RdtscClock.h"
#include "quill/backend/TimestampFormatter.h"
#include "quill/backend/TransitEvent.h"
#include "quill/backend/TransitEventBuffer.h"

#include "quill/core/Attributes.h"
#include "quill/core/BoundedSPSCQueue.h"
#include "quill/core/Codec.h"
#include "quill/core/Common.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/FormatBuffer.h"
#include "quill/core/LogLevel.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/LoggerManager.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/QuillError.h"
#include "quill/core/SinkManager.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/core/ThreadUtilities.h"
#include "quill/core/TimeUtilities.h"
#include "quill/core/UnboundedSPSCQueue.h"
#include "quill/sinks/Sink.h"

#include "quill/bundled/fmt/core.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <exception>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <ratio>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace quill::detail
{
class BackendWorker
{
public:
  /**
   * Constructor
   */
  BackendWorker() { _process_id = std::to_string(get_process_id()); }

  /**
   * Deleted
   */
  BackendWorker(BackendWorker const&) = delete;
  BackendWorker& operator=(BackendWorker const&) = delete;

  /**
   * Destructor
   */
  ~BackendWorker()
  {
    // This destructor will run during static destruction as the thread is part of the singleton
    stop();
  }

  /***/
  QUILL_NODISCARD bool is_running() const noexcept
  {
    return _is_backend_worker_running.load(std::memory_order_relaxed);
  }

  /**
   * Access the rdtsc class to convert an rdtsc value to wall time
   */
  QUILL_NODISCARD uint64_t time_since_epoch(uint64_t rdtsc_value) const
  {
    if (QUILL_UNLIKELY(_backend_options.sleep_duration > _backend_options.rdtsc_resync_interval))
    {
      QUILL_THROW(
        QuillError{"Invalid config, When TSC clock is used backend_thread_sleep_duration should "
                   "not be higher than rdtsc_resync_interval"});
    }

    RdtscClock const* rdtsc_clock = _rdtsc_clock.load(std::memory_order_acquire);
    return rdtsc_clock ? rdtsc_clock->time_since_epoch_safe(rdtsc_value) : 0;
  }

  /**
   * Get the backend worker's thread id
   * @return the backend worker's thread id
   */
  QUILL_NODISCARD uint32_t get_backend_thread_id() const noexcept
  {
    return _backend_worker_thread_id.load();
  }

  /**
   * Starts the backend worker thread
   * @throws std::runtime_error, std::system_error on failures
   */
  QUILL_ATTRIBUTE_COLD void run(BackendOptions const& options)
  {
    _backend_options = options;

    std::thread worker(
      [this]()
      {
        QUILL_TRY
        {
          if (_backend_options.backend_cpu_affinity != (std::numeric_limits<uint16_t>::max)())
          {
            // Set cpu affinity if requested to cpu _backend_thread_cpu_affinity
            set_cpu_affinity(_backend_options.backend_cpu_affinity);
          }
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e) { _backend_options.error_notifier(e.what()); }
        QUILL_CATCH_ALL()
        {
          _backend_options.error_notifier(std::string{"Caught unhandled exception."});
        }
#endif

        QUILL_TRY
        {
          // Set the thread name to the desired name
          set_thread_name(_backend_options.thread_name.data());
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e) { _backend_options.error_notifier(e.what()); }
        QUILL_CATCH_ALL()
        {
          _backend_options.error_notifier(std::string{"Caught unhandled exception."});
        }
#endif

        // Cache this thread's id
        _backend_worker_thread_id.store(get_thread_id());

        // Double check or modify some backend options before we start
        if (_backend_options.transit_events_hard_limit == 0)
        {
          // transit_events_hard_limit of 0 makes no sense as we can't process anything
          _backend_options.transit_events_hard_limit = 1;
        }

        if (_backend_options.transit_events_soft_limit == 0)
        {
          _backend_options.transit_events_soft_limit = 1;
        }

        // All okay, set the backend worker thread running flag
        _is_backend_worker_running.store(true);

        // Running
        while (QUILL_LIKELY(_is_backend_worker_running.load(std::memory_order_relaxed)))
        {
          // main loop
          QUILL_TRY { _main_loop(); }
#if !defined(QUILL_NO_EXCEPTIONS)
          QUILL_CATCH(std::exception const& e) { _backend_options.error_notifier(e.what()); }
          QUILL_CATCH_ALL()
          {
            _backend_options.error_notifier(std::string{"Caught unhandled exception."});
          } // clang-format on
#endif
        }

        // exit
        QUILL_TRY { _exit(); }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e) { _backend_options.error_notifier(e.what()); }
        QUILL_CATCH_ALL()
        {
          _backend_options.error_notifier(std::string{"Caught unhandled exception."});
        } // clang-format on
#endif
      });

    // Move the worker ownership to our class
    _backend_worker.swap(worker);

    while (!_is_backend_worker_running.load(std::memory_order_seq_cst))
    {
      // wait for the thread to start
      std::this_thread::sleep_for(std::chrono::microseconds{100});
    }
  }

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop() noexcept
  {
    // Stop the backend worker
    if (!_is_backend_worker_running.exchange(false))
    {
      // already stopped
      return;
    }

    // signal wake up the backend worker thread
    notify();

    // Wait the backend thread to join, if backend thread was never started it won't be joinable so we can still
    if (_backend_worker.joinable())
    {
      _backend_worker.join();
    }
  }

  /**
   * Wakes up the backend worker thread.
   * Thread safe to be called from any thread
   */
  void notify()
  {
    // Set the flag to indicate that the data is ready
    {
      std::lock_guard<std::mutex> lock{_wake_up_mutex};
      _wake_up = true;
    }

    // Signal the condition variable to wake up the worker thread
    _wake_up_cv.notify_one();
  }

private:
  /**
   * Backend worker thread main function
   */
  QUILL_ATTRIBUTE_HOT void _main_loop()
  {
    // load all contexts locally
    _update_active_thread_contexts_cache();

    size_t total_events{0};

    auto const [tevents, max_events] = _populate_transit_event_buffer();
    total_events = tevents;

    if ((total_events != 0))
    {
      // there are buffered events to process
      if (total_events >= _backend_options.transit_events_soft_limit)
      {
        // we can log only up to max_events, then we want to re-read the queue to avoid
        // logging out of order messages
        for (size_t i = 0; i < (max_events - 1); ++i)
        {
          _process_transit_events();
        }
      }
      else
      {
        // process a single transit event, then give priority to the hot thread spsc queue again
        _process_transit_events();
      }
    }

    if (total_events == 0)
    {
      // None of the thread local queues had any events to process, this means we have processed
      // all messages in all queues We force flush all remaining messages
      _flush_and_run_active_sinks_loop(true);

      // check for any dropped messages / blocked threads
      _check_failure_counter(_backend_options.error_notifier);

      // We can also clear any invalidated or empty thread contexts
      _cleanup_invalidated_thread_contexts();

      // resync rdtsc clock before going to sleep.
      // This is useful when quill::Clock is used
      _resync_rdtsc_clock();

      // Also check if all queues are empty as we need to know that to remove any unused Loggers
      bool all_queues_empty = _check_all_queues_empty();

      if (all_queues_empty)
      {
        // since there are no messages we can check for invalidated loggers and clean them up
        std::vector<std::string> const removed_loggers = _logger_manager.cleanup_invalidated_loggers(
          [this]()
          {
            // we need to reload all thread contexts and check again for empty queues before remove a logger to avoid race condition
            _update_active_thread_contexts_cache();
            return _check_all_queues_empty();
          });

        if (!removed_loggers.empty())
        {
          // if loggers were removed also check for sinks to remove
          // cleanup_unused_sinks is expensive and should be only called when it is needed
          _sink_manager.cleanup_unused_sinks();

          // Clean up any expired pattern_formatters
          for (auto it = _pattern_formatters.begin(); it != _pattern_formatters.end();)
          {
            if (it->expired())
            {
              it = _pattern_formatters.erase(it);
            }
            else
            {
              ++it;
            }
          }

          // Clean up any backtrace storage
          for (auto const& logger_name : removed_loggers)
          {
            _backtrace_storage.erase(logger_name);
          }
        }

        // There is nothing left to do, and we can let this thread sleep for a while
        // buffer events are 0 here and also all the producer queues are empty
        if (_backend_options.sleep_duration.count() != 0)
        {
          std::unique_lock<std::mutex> lock{_wake_up_mutex};

          // Wait for a timeout or a notification to wake up
          _wake_up_cv.wait_for(lock, _backend_options.sleep_duration, [this] { return _wake_up; });

          // set the flag back to false since we woke up here
          _wake_up = false;

          // After waking up resync rdtsc clock again and resume
          _resync_rdtsc_clock();
        }
        else if (_backend_options.enable_yield_when_idle)
        {
          std::this_thread::yield();
        }
      }
    }
  }

  /**
   * Logging thread exit function that flushes everything after stop() is called
   */
  QUILL_ATTRIBUTE_COLD void _exit()
  {
    // load all contexts locally
    _update_active_thread_contexts_cache();

    while (true)
    {
      size_t total_events{0};

      auto const [tevents, max_events] = _populate_transit_event_buffer();
      total_events = tevents;

      if ((total_events != 0))
      {
        // there are buffered events to process
        if (total_events >= _backend_options.transit_events_soft_limit)
        {
          // we can log only up to max_events, then we want to re-read the queue to avoid
          // logging out of order messages
          for (size_t i = 0; i < (max_events - 1); ++i)
          {
            _process_transit_events();
          }
        }
        else
        {
          // process a single transit event, then give priority to the hot thread spsc queue again
          _process_transit_events();
        }
      }

      if (total_events == 0)
      {
        bool all_empty{true};

        if (_backend_options.wait_for_queues_to_empty_before_exit)
        {
          all_empty = _check_all_queues_empty();
        }

        if (all_empty)
        {
          // we are done, all queues are now empty
          _check_failure_counter(_backend_options.error_notifier);
          _flush_and_run_active_sinks_loop(false);
          break;
        }
      }
    }

    RdtscClock const* rdtsc_clock{_rdtsc_clock.load(std::memory_order_relaxed)};
    _rdtsc_clock.store(nullptr, std::memory_order_release);
    delete rdtsc_clock;
  }

  /**
   * Populates the local transit event buffer
   */
  QUILL_ATTRIBUTE_HOT std::pair<size_t, size_t> _populate_transit_event_buffer()
  {
    uint64_t const ts_now = _backend_options.enable_strict_log_timestamp_order
      ? static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count())
      : 0;

    size_t total_events{0};
    size_t max_events{0};

    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      uint32_t events{0};

      assert(thread_context->has_unbounded_queue_type() || thread_context->has_bounded_queue_type());

      if (thread_context->has_unbounded_queue_type())
      {
        // copy everything from the SPSC queue to the transit event buffer to process it later
        events = _read_queue_messages_and_decode(
          thread_context->get_spsc_queue_union().unbounded_spsc_queue, thread_context, ts_now);
      }
      else if (thread_context->has_bounded_queue_type())
      {
        // copy everything from the SPSC queue to the transit event buffer to process it later
        events = _read_queue_messages_and_decode(
          thread_context->get_spsc_queue_union().bounded_spsc_queue, thread_context, ts_now);
      }

      total_events += events;

      if (events > max_events)
      {
        max_events = events;
      }
    }

    return std::make_pair(total_events, max_events);
  }

  /**
   * Deserialize messages from the raw SPSC queue
   * @param queue queue
   * @param thread_context thread context
   * @param ts_now timestamp now
   * @return total events stored in the transit_event_buffer
   */
  template <typename QueueT>
  QUILL_ATTRIBUTE_HOT uint32_t _read_queue_messages_and_decode(QueueT& queue, ThreadContext* thread_context,
                                                               uint64_t ts_now)
  {
    // Note: The producer will commit to this queue when one complete message is written.
    // This means that if we can read something from the queue it will be a full message

    if (!thread_context->_transit_event_buffer)
    {
      thread_context->_transit_event_buffer =
        std::make_shared<UnboundedTransitEventBuffer>(_backend_options.transit_event_buffer_initial_capacity);
    }

    size_t const queue_capacity = queue.capacity();
    uint32_t total_bytes_read{0};

    std::byte* read_pos;
    if constexpr (std::is_same_v<QueueT, UnboundedSPSCQueue>)
    {
      read_pos = _read_unbounded_queue(queue, thread_context);
    }
    else
    {
      read_pos = queue.prepare_read();
    }

    // read max of one full queue and also max_transit events otherwise we can get stuck here
    // forever if the producer keeps producing
    while ((total_bytes_read < queue_capacity) && read_pos)
    {
      if (thread_context->_transit_event_buffer->size() == _backend_options.transit_events_hard_limit)
      {
        // stop reading the queue, we reached the transit buffer hard limit
        return thread_context->_transit_event_buffer->size();
      }

      std::byte const* const read_begin = read_pos;

      if (!_get_transit_event_from_queue(read_pos, thread_context, ts_now))
      {
        // if _get_transit_event_from_queue returns false we stop reading
        return thread_context->_transit_event_buffer->size();
      }

      // Finish reading
      assert((read_pos >= read_begin) && "read_buffer should be greater or equal to read_begin");
      queue.finish_read(static_cast<uint32_t>(read_pos - read_begin));
      total_bytes_read += static_cast<uint32_t>(read_pos - read_begin);

      // read again
      if constexpr (std::is_same_v<QueueT, UnboundedSPSCQueue>)
      {
        read_pos = _read_unbounded_queue(queue, thread_context);
      }
      else
      {
        read_pos = queue.prepare_read();
      }
    }

    if (total_bytes_read != 0)
    {
      // we read something from the queue, we commit all the reads together at the end
      queue.commit_read();
    }

    return thread_context->_transit_event_buffer->size();
  }

  /***/
  QUILL_ATTRIBUTE_HOT bool _get_transit_event_from_queue(std::byte*& read_pos,
                                                         ThreadContext* thread_context, uint64_t ts_now)
  {
    // First we want to allocate a new TransitEvent or use an existing one
    // to store the message from the queue
    assert(thread_context->_transit_event_buffer);

    TransitEvent* transit_event = thread_context->_transit_event_buffer->back();
    transit_event->thread_id = thread_context->thread_id();
    transit_event->thread_name = thread_context->thread_name();

    // read the header first, and take copy of the header
    std::memcpy(&transit_event->timestamp, read_pos, sizeof(transit_event->timestamp));
    read_pos += sizeof(uint64_t);

    std::memcpy(&transit_event->macro_metadata, read_pos, sizeof(uintptr_t));
    read_pos += sizeof(uintptr_t);

    LoggerBase* logger_base;
    std::memcpy(&logger_base, read_pos, sizeof(uintptr_t));
    read_pos += sizeof(uintptr_t);

    // Look up to see if we have the formatter and if not create it
    if (!logger_base->pattern_formatter)
    {
      // Search for an existing pattern_formatter
      auto search_it = std::find_if(
        _pattern_formatters.begin(), _pattern_formatters.end(),
        [logger_base](std::weak_ptr<PatternFormatter> const& elem)
        {
          std::shared_ptr<PatternFormatter> pattern_formatter = elem.lock();

          return pattern_formatter && (pattern_formatter->format_pattern() == logger_base->format_pattern) &&
            (pattern_formatter->timestamp_formatter().time_format() == logger_base->time_pattern) &&
            (pattern_formatter->timestamp_formatter().timestamp_timezone() == logger_base->timezone);
        });

      if (search_it != std::cend(_pattern_formatters))
      {
        // we found a pattern formatter we can use. We are the only thread removing loggers and
        // we know it is safe to re-lock the weak pointer will still be valid
        assert(!search_it->expired());
        logger_base->pattern_formatter = search_it->lock();
      }
      else
      {
        // need to create a new pattern formatter
        logger_base->pattern_formatter = std::make_shared<PatternFormatter>(
          logger_base->format_pattern, logger_base->time_pattern, logger_base->timezone);
        _pattern_formatters.push_back(logger_base->pattern_formatter);
      }
    }

    transit_event->logger_base = logger_base;

    std::memcpy(&transit_event->format_args_decoder, read_pos, sizeof(uintptr_t));
    read_pos += sizeof(uintptr_t);

    // if we are using rdtsc clock then here we will convert the value to nanoseconds since epoch
    // doing the conversion here ensures that every transit that is inserted in the transit buffer
    // below has a header timestamp of nanoseconds since epoch and makes it even possible to
    // have Logger objects using different clocks
    if (transit_event->logger_base->clock_source == ClockSourceType::Tsc)
    {
      if (!_rdtsc_clock.load(std::memory_order_relaxed))
      {
        // Here we lazy initialise rdtsc clock on the backend thread only if the user decides to use it
        // Use rdtsc clock based on config. The clock requires a few seconds to init as it is
        // taking samples first
        _rdtsc_clock.store(new RdtscClock{_backend_options.rdtsc_resync_interval}, std::memory_order_release);
        _last_rdtsc_resync = std::chrono::system_clock::now();
      }

      // convert the rdtsc value to nanoseconds since epoch
      transit_event->timestamp =
        _rdtsc_clock.load(std::memory_order_relaxed)->time_since_epoch(transit_event->timestamp);

      // Now check if the message has a timestamp greater than our ts_now
      if (QUILL_UNLIKELY((ts_now != 0) && ((transit_event->timestamp / 1'000) >= ts_now)))
      {
        // We are reading the queues sequentially and to be fair when ordering the messages
        // we are trying to avoid the situation when we already read the first queue,
        // and then we missed it when reading the last queue

        // if the message timestamp is greater than our timestamp then we stop reading this queue
        // for now and we will continue in the next circle

        // we return here and never call transit_event_buffer.push_back();
        return false;
      }
    }
    else if (transit_event->logger_base->clock_source == ClockSourceType::System)
    {
      if (QUILL_UNLIKELY((ts_now != 0) && ((transit_event->timestamp / 1'000) >= ts_now)))
      {
        // We are reading the queues sequentially and to be fair when ordering the messages
        // we are trying to avoid the situation when we already read the first queue,
        // and then we missed it when reading the last queue

        // if the message timestamp is greater than our timestamp then we stop reading this queue
        // for now and we will continue in the next circle

        // we return here and never call transit_event_buffer.push_back();
        return false;
      }
    }
    else if (transit_event->logger_base->clock_source == ClockSourceType::User)
    {
      // we skip checking against `ts_now`, we can not compare a custom timestamp by
      // the user (TimestampClockType::Custom) against ours
    }

    // we need to check and do not try to format the flush events as that wouldn't be valid
    if (transit_event->macro_metadata->event() != MacroMetadata::Event::Flush)
    {
      // lib fmt style logs
      auto format_args_decoder = reinterpret_cast<detail::FormatArgsDecoder>(transit_event->format_args_decoder);
      assert(format_args_decoder);

      if (transit_event->macro_metadata->is_structured_log_template())
      {
        if (!transit_event->structured_params)
        {
          transit_event->structured_params =
            std::make_unique<std::vector<std::pair<std::string, std::string>>>();
        }
        else
        {
          transit_event->structured_params->clear();
        }

        // using the message_format as key for lookups
        _structured_fmt_str.assign(transit_event->macro_metadata->message_format());

        // for messages containing named arguments threat them as structured logs
        if (auto const search = _structured_log_templates.find(_structured_fmt_str);
            search != std::cend(_structured_log_templates))
        {
          // process structured log that structured keys exist in our map
          auto const& [fmt_str, structured_keys] = search->second;

          transit_event->structured_params->resize(structured_keys.size());

          // We first populate the structured keys in the transit buffer
          for (size_t i = 0; i < structured_keys.size(); ++i)
          {
            (*transit_event->structured_params)[i].first = structured_keys[i];
          }

          format_args_decoder(read_pos, _format_args_store);

          transit_event->formatted_msg.clear();

          QUILL_TRY
          {
            fmtquill::vformat_to(std::back_inserter(transit_event->formatted_msg), fmt_str,
                                 fmtquill::basic_format_args<fmtquill::format_context>{
                                   _format_args_store.get_types(), _format_args_store.data()});

            // format the values of each key
            _format_and_split_arguments(*transit_event->structured_params, _format_args_store);
          }
#if !defined(QUILL_NO_EXCEPTIONS)
          QUILL_CATCH(std::exception const& e)
          {
            transit_event->formatted_msg.clear();
            std::string const error = fmtquill::format(
              "[Could not format log statement. message: \"{}\", location: \"{}\", error: "
              "\"{}\"]",
              transit_event->macro_metadata->message_format(),
              transit_event->macro_metadata->short_source_location(), e.what());

            transit_event->formatted_msg.append(error);
            _backend_options.error_notifier(error);
          }
#endif
        }
        else
        {
          // process structured log that structured keys are processed for the first time
          // parse structured keys and stored them to our lookup map
          auto const [res_it, inserted] = _structured_log_templates.try_emplace(
            _structured_fmt_str,
            _process_structured_log_template(transit_event->macro_metadata->message_format()));

          // suppress unused warning
          (void)inserted;

          auto const& [fmt_str, structured_keys] = res_it->second;

          transit_event->structured_params->resize(structured_keys.size());

          // We first populate the structured keys in the transit buffer
          for (size_t i = 0; i < structured_keys.size(); ++i)
          {
            (*transit_event->structured_params)[i].first = structured_keys[i];
          }

          format_args_decoder(read_pos, _format_args_store);

          transit_event->formatted_msg.clear();

          QUILL_TRY
          {
            fmtquill::vformat_to(std::back_inserter(transit_event->formatted_msg), fmt_str,
                                 fmtquill::basic_format_args<fmtquill::format_context>{
                                   _format_args_store.get_types(), _format_args_store.data()});

            // format the values of each key
            _format_and_split_arguments(*transit_event->structured_params, _format_args_store);
          }
#if !defined(QUILL_NO_EXCEPTIONS)
          QUILL_CATCH(std::exception const& e)
          {
            transit_event->formatted_msg.clear();
            std::string const error = fmtquill::format(
              "[Could not format log statement. message: \"{}\", location: \"{}\", error: "
              "\"{}\"]",
              transit_event->macro_metadata->message_format(),
              transit_event->macro_metadata->short_source_location(), e.what());

            transit_event->formatted_msg.append(error);
            _backend_options.error_notifier(error);
          }
#endif
        }
      }
      else
      {
        format_args_decoder(read_pos, _format_args_store);

        transit_event->formatted_msg.clear();

        QUILL_TRY
        {
          fmtquill::vformat_to(std::back_inserter(transit_event->formatted_msg),
                               transit_event->macro_metadata->message_format(),
                               fmtquill::basic_format_args<fmtquill::format_context>{
                                 _format_args_store.get_types(), _format_args_store.data()});
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e)
        {
          transit_event->formatted_msg.clear();
          std::string const error = fmtquill::format(
            "[Could not format log statement. message: \"{}\", location: \"{}\", error: \"{}\"]",
            transit_event->macro_metadata->message_format(),
            transit_event->macro_metadata->short_source_location(), e.what());

          transit_event->formatted_msg.append(error);
          _backend_options.error_notifier(error);
        }
#endif
      }

      if (transit_event->macro_metadata->log_level() == LogLevel::Dynamic)
      {
        // if this is a dynamic log level we need to read the log level from the buffer
        LogLevel dynamic_log_level;
        std::memcpy(&dynamic_log_level, read_pos, sizeof(dynamic_log_level));
        read_pos += sizeof(LogLevel);

        // Also set the dynamic log level to the transit event
        transit_event->log_level_override = dynamic_log_level;
      }
      else
      {
        // Important: if a dynamic log level is not being used, then this must
        // not have a value, otherwise the wrong log level may be used later.
        // We can't assume that this member (or any member of TransitEvent) has
        // its default value because TransitEvents may be reused.
        transit_event->log_level_override = std::nullopt;
      }
    }
    else
    {
      // if this is a flush event then we do not need to format anything for the
      // transit_event, but we need to set the transit event's flush_flag pointer instead
      uintptr_t flush_flag_tmp;
      std::memcpy(&flush_flag_tmp, read_pos, sizeof(uintptr_t));
      transit_event->flush_flag = reinterpret_cast<std::atomic<bool>*>(flush_flag_tmp);
      read_pos += sizeof(uintptr_t);
    }

    // commit this transit event
    thread_context->_transit_event_buffer->push_back();

    return true;
  }

  /**
   * Checks for events in all queues and processes the one with the minimum timestamp
   */
  QUILL_ATTRIBUTE_HOT void _process_transit_events()
  {
    // Get the lowest timestamp
    uint64_t min_ts{std::numeric_limits<uint64_t>::max()};
    UnboundedTransitEventBuffer* transit_buffer{nullptr};

    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      if (thread_context->_transit_event_buffer)
      {
        if (TransitEvent const* te = thread_context->_transit_event_buffer->front(); te && (min_ts > te->timestamp))
        {
          min_ts = te->timestamp;
          transit_buffer = thread_context->_transit_event_buffer.get();
        }
      }
    }

    if (!transit_buffer)
    {
      // all buffers are empty
      // return false, meaning we processed a message
      return;
    }

    TransitEvent* transit_event = transit_buffer->front();
    assert(transit_event && "transit_buffer is set only when transit_event is valid");

    QUILL_TRY { _process_transit_event(*transit_event); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH(std::exception const& e) { _backend_options.error_notifier(e.what()); }
    QUILL_CATCH_ALL()
    {
      _backend_options.error_notifier(std::string{"Caught unhandled exception."});
    } // clang-format on
#endif

    // Remove this event and move to the next.
    transit_buffer->pop_front();
  }

  /**
   * Process a single trnasit event
   */
  QUILL_ATTRIBUTE_HOT void _process_transit_event(TransitEvent& transit_event)
  {
    // If backend_process(...) throws we want to skip this event and move to the next, so we catch
    // the error here instead of catching it in the parent try/catch block of main_loop
    if (transit_event.macro_metadata->event() == MacroMetadata::Event::Log)
    {
      if (transit_event.log_level() != LogLevel::Backtrace)
      {
        _write_transit_event(transit_event);

        // We also need to check the severity of the log message here against the backtrace
        // Check if we should also flush the backtrace messages:
        // After we forwarded the message we will check the severity of this message for this logger
        // If the severity of the message is higher than the backtrace flush severity we will also
        // flush the backtrace of the logger
        if (QUILL_UNLIKELY(transit_event.log_level() >=
                           transit_event.logger_base->backtrace_flush_level.load(std::memory_order_relaxed)))
        {
          _backtrace_storage.process(transit_event.logger_base->logger_name,
                                     [this](TransitEvent const& te) { _write_transit_event(te); });
        }
      }
      else
      {
        // this is a backtrace log and we will store it
        _backtrace_storage.store(std::move(transit_event));
      }
    }
    else if (transit_event.macro_metadata->event() == MacroMetadata::Event::InitBacktrace)
    {
      // we can just convert the capacity back to int here and use it
      _backtrace_storage.set_capacity(
        transit_event.logger_base->logger_name,
        static_cast<uint32_t>(std::stoul(
          std::string{transit_event.formatted_msg.begin(), transit_event.formatted_msg.end()})));
    }
    else if (transit_event.macro_metadata->event() == MacroMetadata::Event::FlushBacktrace)
    {
      // process all records in backtrace for this logger_name and log them by calling backend_process_backtrace_log_message
      _backtrace_storage.process(transit_event.logger_base->logger_name,
                                 [this](TransitEvent const& te) { _write_transit_event(te); });
    }
    else if (transit_event.macro_metadata->event() == MacroMetadata::Event::Flush)
    {
      _flush_and_run_active_sinks_loop(false);

      // this is a flush event, so we need to notify the caller to continue now
      transit_event.flush_flag->store(true);

      // we also need to reset the flush_flag as the TransitEvents are re-used
      transit_event.flush_flag = nullptr;
    }
  }

  /**
   * Write a transit event
   */
  QUILL_ATTRIBUTE_HOT void _write_transit_event(TransitEvent const& transit_event) const
  {
    std::string_view const formatted_log_message = transit_event.logger_base->pattern_formatter->format(
      transit_event.timestamp, transit_event.thread_id, transit_event.thread_name, _process_id,
      transit_event.logger_base->logger_name, loglevel_to_string(transit_event.log_level()),
      *transit_event.macro_metadata, transit_event.structured_params.get(),
      std::string_view{transit_event.formatted_msg.data(), transit_event.formatted_msg.size()});

    for (auto& sink : transit_event.logger_base->sinks)
    {
      // If all filters are okay we write this message to the file
      if (sink->apply_all_filters(transit_event.macro_metadata, transit_event.timestamp,
                                  transit_event.thread_id, transit_event.thread_name,
                                  transit_event.logger_base->logger_name, transit_event.log_level(),
                                  formatted_log_message))
      {
        sink->write_log_message(transit_event.macro_metadata, transit_event.timestamp,
                                transit_event.thread_id, transit_event.thread_name,
                                transit_event.logger_base->logger_name, transit_event.log_level(),
                                transit_event.structured_params.get(), formatted_log_message);
      }
    }
  }

  /**
   * Check for dropped messages - only when bounded queue is used
   * @param error_notifier error notifier
   */
  QUILL_ATTRIBUTE_HOT void _check_failure_counter(std::function<void(std::string const&)> const& error_notifier) noexcept
  {
    // UnboundedNoMaxLimit does not block or drop messages
    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      size_t const failed_messages_cnt = thread_context->get_and_reset_failure_counter();

      if (QUILL_UNLIKELY(failed_messages_cnt > 0))
      {
        char timestamp[24];
        time_t now = time(nullptr);
        tm local_time;
        localtime_rs(&now, &local_time);
        strftime(timestamp, sizeof(timestamp), "%X", &local_time);

        if (thread_context->has_dropping_queue())
        {
          error_notifier(fmtquill::format("{} Quill INFO: Dropped {} log messages from thread {}",
                                          timestamp, failed_messages_cnt, thread_context->thread_id()));
        }
        else if (thread_context->has_blocking_queue())
        {
          error_notifier(
            fmtquill::format("{} Quill INFO: Experienced {} blocking occurrences on thread {}",
                             timestamp, failed_messages_cnt, thread_context->thread_id()));
        }
      }
    }
  }

  /**
   * Process a structured log template message
   * @param fmt_template a structured log template message containing named arguments
   * @return first: fmt string without the named arguments, second: a vector extracted keys
   */
  QUILL_ATTRIBUTE_HOT static std::pair<std::string, std::vector<std::string>> _process_structured_log_template(
    std::string_view fmt_template) noexcept
  {
    // It would be nice to do this at compile time and store it in macro metadata, but without
    // constexpr vector and string in c++17 it is not possible
    std::string fmt_str;
    std::vector<std::string> keys;

    size_t cur_pos = 0;

    size_t open_bracket_pos = fmt_template.find_first_of('{');
    while (open_bracket_pos != std::string::npos)
    {
      // found an open bracket
      if (size_t const open_bracket_2_pos = fmt_template.find_first_of('{', open_bracket_pos + 1);
          open_bracket_2_pos != std::string::npos)
      {
        // found another open bracket
        if ((open_bracket_2_pos - 1) == open_bracket_pos)
        {
          open_bracket_pos = fmt_template.find_first_of('{', open_bracket_2_pos + 1);
          continue;
        }
      }

      // look for the next close bracket
      size_t close_bracket_pos = fmt_template.find_first_of('}', open_bracket_pos + 1);
      while (close_bracket_pos != std::string::npos)
      {
        // found closed bracket
        if (size_t const close_bracket_2_pos = fmt_template.find_first_of('}', close_bracket_pos + 1);
            close_bracket_2_pos != std::string::npos)
        {
          // found another open bracket
          if ((close_bracket_2_pos - 1) == close_bracket_pos)
          {
            close_bracket_pos = fmt_template.find_first_of('}', close_bracket_2_pos + 1);
            continue;
          }
        }

        // construct a fmt string excluding the characters inside the brackets { }
        fmt_str += std::string{fmt_template.substr(cur_pos, open_bracket_pos - cur_pos)} + "{}";
        cur_pos = close_bracket_pos + 1;

        // also add the keys to the vector
        keys.emplace_back(fmt_template.substr(open_bracket_pos + 1, (close_bracket_pos - open_bracket_pos - 1)));

        break;
      }

      open_bracket_pos = fmt_template.find_first_of('{', close_bracket_pos);
    }

    // add anything remaining after the last bracket
    fmt_str += std::string{fmt_template.substr(cur_pos, fmt_template.length() - cur_pos)};
    return std::make_pair(fmt_str, keys);
  }

  /**
   * Helper function to read the unbounded queue and also report the allocation
   * @param queue queue
   * @param thread_context thread context
   * @return start position of read
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* _read_unbounded_queue(UnboundedSPSCQueue& queue,
                                                                       ThreadContext* thread_context) const
  {
    auto const read_result = queue.prepare_read();

    if (read_result.allocation)
    {
      // When allocation_info has a value it means that the queue has re-allocated
      if (_backend_options.error_notifier)
      {
        char ts[24];
        time_t t = time(nullptr);
        tm p;
        localtime_rs(std::addressof(t), std::addressof(p));
        strftime(ts, 24, "%X", std::addressof(p));

        // we switched to a new here, and we also notify the user of the allocation via the
        // error_notifier
        _backend_options.error_notifier(fmtquill::format(
          "{} Quill INFO: A new SPSC queue has been allocated with a new capacity of {} bytes and "
          "a previous capacity of {} bytes from thread {}",
          ts, read_result.new_capacity, read_result.previous_capacity, thread_context->thread_id()));
      }
    }

    return read_result.read_pos;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool _check_all_queues_empty()
  {
    bool all_empty{true};

    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      assert(thread_context->has_unbounded_queue_type() || thread_context->has_bounded_queue_type());

      if (thread_context->has_unbounded_queue_type())
      {
        all_empty &= thread_context->get_spsc_queue_union().unbounded_spsc_queue.empty();
      }
      else if (thread_context->has_bounded_queue_type())
      {
        all_empty &= thread_context->get_spsc_queue_union().bounded_spsc_queue.empty();
      }

      if (thread_context->_transit_event_buffer)
      {
        all_empty &= thread_context->_transit_event_buffer->empty();
      }
    }

    return all_empty;
  }

  /**
   * Resyncs the rdtsc clock
   */
  QUILL_ATTRIBUTE_HOT void _resync_rdtsc_clock()
  {
    if (_rdtsc_clock.load(std::memory_order_relaxed))
    {
      // resync in rdtsc if we are not logging so that time_since_epoch() still works

      if (auto const now = std::chrono::system_clock::now();
          (now - _last_rdtsc_resync) > _backend_options.rdtsc_resync_interval)
      {
        if (_rdtsc_clock.load(std::memory_order_relaxed)->resync(2500))
        {
          _last_rdtsc_resync = now;
        }
      }
    }
  }

  /**
   * Updates the active sinks cache
   */
  void _flush_and_run_active_sinks_loop(bool run_loop)
  {
    // Update the active sinks cache, consider only the valid loggers
    _logger_manager.for_each_logger(
      [this](LoggerBase* logger)
      {
        _active_sinks_cache.clear();

        if (logger->is_valid_logger())
        {
          for (std::shared_ptr<Sink> const& sink : logger->sinks)
          {
            auto search_it = std::find_if(_active_sinks_cache.begin(), _active_sinks_cache.end(),
                                          [sink_ptr = sink.get()](std::weak_ptr<Sink> const& elem)
                                          {
                                            // no one else can remove the shared pointer as this is only
                                            // running on backend thread, lock() will always succeed
                                            return elem.lock().get() == sink_ptr;
                                          });

            if (search_it == std::end(_active_sinks_cache))
            {
              _active_sinks_cache.push_back(sink);
            }
          }
        }
      });

    for (auto const& sink : _active_sinks_cache)
    {
      std::shared_ptr<Sink> h = sink.lock();
      if (h)
      {
        QUILL_TRY
        {
          // If an exception is thrown, catch it here to prevent it from propagating
          // to the outer function. This prevents potential infinite loops caused by failing flush operations.
          h->flush_sink();
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e) { _backend_options.error_notifier(e.what()); }
        QUILL_CATCH_ALL()
        {
          _backend_options.error_notifier(std::string{"Caught unhandled exception."});
        }
#endif

        if (run_loop)
        {
          h->run_periodic_tasks();
        }
      }
    }
  }

  /**
   * Reloads the thread contexts in our local cache.
   * Any invalidated thread contexts with empty queues are removed and any new thread contexts
   * from new threads are added to the returned vector of caches.
   * If there are no invalidated contexts or no new contexts the existing cache is returned
   */
  QUILL_ATTRIBUTE_HOT void _update_active_thread_contexts_cache()
  {
    // Check if _thread_contexts has changed. This can happen only when a new thread context is added by any Logger
    if (QUILL_UNLIKELY(_thread_context_manager.new_thread_context_flag()))
    {
      _active_thread_contexts_cache.clear();
      _thread_context_manager.for_each_thread_context(
        [this](ThreadContext* thread_context)
        {
          // We do not skip invalidated && empty queue thread contexts as this is very rare, so instead we just add them and expect them to be cleaned in the next iteration
          _active_thread_contexts_cache.push_back(thread_context);
        });
    }
  }

  /**
   * Looks into the _thread_context_cache and removes all thread contexts that are 1) invalidated
   * and 2) have an empty queue of no events to process
   *
   * @note Only called by the backend thread
   */
  void _cleanup_invalidated_thread_contexts()
  {
    if (!_thread_context_manager.has_invalid_thread_context())
    {
      return;
    }

    auto find_invalid_and_empty_thread_context_callback = [](ThreadContext* thread_context)
    {
      // If the thread context is invalid it means the thread that created it has now died.
      // We also want to empty the queue from all LogRecords before removing the thread context
      if (!thread_context->is_valid_context())
      {
        bool has_empty_queue{false};

        assert(thread_context->has_unbounded_queue_type() || thread_context->has_bounded_queue_type());

        // detect empty queue
        if (thread_context->has_unbounded_queue_type())
        {
          has_empty_queue = thread_context->get_spsc_queue_union().unbounded_spsc_queue.empty();
        }
        else if (thread_context->has_bounded_queue_type())
        {
          has_empty_queue = thread_context->get_spsc_queue_union().bounded_spsc_queue.empty();
        }

        if (has_empty_queue)
        {
          if (thread_context->_transit_event_buffer)
          {
            if (thread_context->_transit_event_buffer->empty())
            {
              return true;
            }
          }
          else
          {
            // if _transit_event_buffer is not used, checking for the empty queue is enough
            return true;
          }
        }
      }

      return false;
    };

    // First we iterate our existing cache and we look for any invalidated contexts
    auto found_invalid_and_empty_thread_context =
      std::find_if(_active_thread_contexts_cache.begin(), _active_thread_contexts_cache.end(),
                   find_invalid_and_empty_thread_context_callback);

    while (QUILL_UNLIKELY(found_invalid_and_empty_thread_context != std::end(_active_thread_contexts_cache)))
    {
      // if we found anything then remove it - Here if we have more than one to remove we will
      // try to acquire the lock multiple times, but it should be fine as it is unlikely to have
      // that many to remove
      _thread_context_manager.remove_shared_invalidated_thread_context(*found_invalid_and_empty_thread_context);

      // We also need to remove it from _thread_context_cache, that is used only by the backend
      _active_thread_contexts_cache.erase(found_invalid_and_empty_thread_context);

      // And then look again
      found_invalid_and_empty_thread_context =
        std::find_if(_active_thread_contexts_cache.begin(), _active_thread_contexts_cache.end(),
                     find_invalid_and_empty_thread_context_callback);
    }
  }

  /**
   * This function takes an `arg_store` containing multiple arguments and formats them into
   * a single string using a generated format string. Due to limitations in the ability to
   * iterate and format each argument individually in libfmt, this approach is used.
   * After formatting, the string is split to isolate each formatted value.
   */
  void _format_and_split_arguments(std::vector<std::pair<std::string, std::string>>& structured_params,
                                   DynamicFormatArgStore const& arg_store)
  {
    // Generate a format string
    std::string format_string;
    static constexpr std::string_view delimiter{"\x01\x02\x03"};

    for (size_t i = 0; i < structured_params.size(); ++i)
    {
      format_string += "{}";
      if (i < structured_params.size() - 1)
      {
        format_string += delimiter;
      }
    }

    // Format all values to a single string
    std::string formatted_values_str;
    fmtquill::vformat_to(
      std::back_inserter(formatted_values_str), format_string,
      fmtquill::basic_format_args<fmtquill::format_context>{arg_store.get_types(), arg_store.data()});

    // Split the formatted_values to isolate each value
    size_t start = 0;
    size_t end = 0;
    size_t idx = 0;

    while ((end = formatted_values_str.find(delimiter, start)) != std::string::npos)
    {
      if (idx < structured_params.size())
      {
        structured_params[idx++].second = formatted_values_str.substr(start, end - start);
      }
      start = end + delimiter.length();
    }

    if (idx < structured_params.size())
    {
      structured_params[idx].second = formatted_values_str.substr(start);
    }
  }

private:
  ThreadContextManager& _thread_context_manager = ThreadContextManager::instance();
  SinkManager& _sink_manager = SinkManager::instance();
  LoggerManager& _logger_manager = LoggerManager::instance();
  BackendOptions _backend_options;

  std::thread _backend_worker;

  std::atomic<RdtscClock*> _rdtsc_clock{nullptr}; /** rdtsc clock if enabled **/

  DynamicFormatArgStore _format_args_store; /** Format args tmp storage as member to avoid reallocation */
  std::vector<std::weak_ptr<Sink>> _active_sinks_cache;
  std::vector<ThreadContext*> _active_thread_contexts_cache;
  std::vector<std::weak_ptr<PatternFormatter>> _pattern_formatters;

  BacktraceStorage _backtrace_storage; /** Stores a vector of backtrace messages per logger name */
  std::unordered_map<std::string, std::pair<std::string, std::vector<std::string>>> _structured_log_templates; /** Avoid re-formating the same structured template each time */

  /** Id of the current running process **/
  std::string _process_id;
  std::string _structured_fmt_str; /** to avoid allocation each time **/
  std::chrono::system_clock::time_point _last_rdtsc_resync;
  std::atomic<uint32_t> _backend_worker_thread_id{0}; /** cached backend worker thread id */

  std::atomic<bool> _is_backend_worker_running{false}; /** The spawned backend thread status */

  alignas(CACHE_LINE_ALIGNED) std::mutex _wake_up_mutex;
  std::condition_variable _wake_up_cv;
  bool _wake_up{false};
};
} // namespace quill::detail
