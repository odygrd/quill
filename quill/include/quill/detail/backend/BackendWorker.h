/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/Config.h"                   // for Config
#include "quill/QuillError.h"               // for QUILL_CATCH, QUILL...
#include "quill/detail/HandlerCollection.h" // for HandlerCollection
#include "quill/detail/LoggerCollection.h"  // for HandlerCollection
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/Serialize.h"
#include "quill/detail/ThreadContext.h"            // for ThreadContext, Thr...
#include "quill/detail/ThreadContextCollection.h"  // for ThreadContextColle...
#include "quill/detail/backend/BacktraceStorage.h" // for BacktraceStorage
#include "quill/detail/backend/TransitEventBuffer.h"
#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_HOT
#include "quill/detail/misc/Common.h"     // for QUILL_LIKELY
#include "quill/detail/misc/Os.h"         // for set_cpu_affinity, get_thread_id
#include "quill/detail/misc/RdtscClock.h" // for RdtscClock
#include "quill/detail/misc/Utilities.h"
#include "quill/detail/spsc_queue/UnboundedQueue.h"
#include "quill/handlers/Handler.h" // for Handler
#include <atomic>                   // for atomic, memory_ord...
#include <cassert>                  // for assert
#include <chrono>                   // for nanoseconds, milli...
#include <condition_variable>
#include <cstdint>    // for uint16_t
#include <exception>  // for exception
#include <functional> // for greater, function
#include <limits>     // for numeric_limits
#include <memory>     // for unique_ptr, make_u...
#include <mutex>
#include <string>  // for allocator, string
#include <thread>  // for sleep_for, thread
#include <utility> // for move
#include <vector>  // for vector

namespace quill::detail
{

class BackendWorker
{
public:
  /**
   * Constructor
   */
  BackendWorker(Config const& config, ThreadContextCollection& thread_context_collection,
                HandlerCollection& handler_collection, LoggerCollection& logger_collection);

  /**
   * Deleted
   */
  BackendWorker(BackendWorker const&) = delete;
  BackendWorker& operator=(BackendWorker const&) = delete;

  /**
   * Destructor
   */
  ~BackendWorker();

  /**
   * Returns the status of the backend worker thread
   * @return true when the worker is running, false otherwise
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline bool is_running() const noexcept;

  /**
   * Access the rdtsc class to convert an rdtsc value to wall time
   * @param rdtsc_value
   * @return
   */
  QUILL_NODISCARD inline uint64_t time_since_epoch(uint64_t rdtsc_value) const;

  /**
   * Get the backend worker's thread id
   * @return the backend worker's thread id
   */
  QUILL_NODISCARD uint32_t thread_id() const noexcept;

  /**
   * Starts the backend worker thread
   * @throws std::runtime_error, std::system_error on failures
   */
  QUILL_ATTRIBUTE_COLD inline void run();

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop() noexcept;

  /**
   * Wakes up the backend worker thread.
   * Thread safe to be called from any thread
   */
  void wake_up();

private:
  /**
   * Backend worker thread main function
   */
  QUILL_ATTRIBUTE_HOT inline void _main_loop();

  /**
   * Logging thread exit function that flushes everything after stop() is called
   */
  QUILL_ATTRIBUTE_COLD inline void _exit();

  /**
   * Populate our local transit event buffer
   * @param cached_thread_contexts local thread context cache
   * @return total size of all transit event buffers, max of events
   */
  QUILL_ATTRIBUTE_HOT inline std::pair<size_t, size_t> _populate_transit_event_buffer(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts);

  /**
   * Deserialize messages from the raw SPSC queue
   * @param thread_context thread context
   * @param ts_now timestamp now
   * @return total events stored in the transit_event_buffer
   */
  template <typename QueueT>
  QUILL_ATTRIBUTE_HOT inline uint32_t _read_queue_messages_and_decode(QueueT& queue, ThreadContext* thread_context,
                                                                      uint64_t ts_now);

  QUILL_ATTRIBUTE_HOT inline bool _get_transit_event_from_queue(std::byte*& read_pos, ThreadContext* thread_context,
                                                                uint64_t ts_now);

  /**
   * Checks for events in all queues and processes the one with the minimum timestamp
   */
  QUILL_ATTRIBUTE_HOT inline void _process_transit_events(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts);

  /**
   * Process a single trnasit event
   */
  QUILL_ATTRIBUTE_HOT inline void _process_transit_event(TransitEvent& transit_event);

  /**
   * Write a transit event
   */
  QUILL_ATTRIBUTE_HOT inline void _write_transit_event(TransitEvent const& transit_event);

  /**
   * Process the lowest timestamp from the queues and write it to the log file
   */
  QUILL_ATTRIBUTE_HOT inline bool _process_and_write_single_message(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts);

  /**
   * Force flush all active Handlers
   */
  QUILL_ATTRIBUTE_HOT inline void _force_flush(std::vector<std::weak_ptr<Handler>> const& active_handlers);

  /**
   * Check for dropped messages - only when bounded queue is used
   * @param cached_thread_contexts loaded thread contexts
   */
  QUILL_ATTRIBUTE_HOT inline static void _check_message_failures(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts,
    backend_worker_notification_handler_t const& notification_handler) noexcept;

  /**
   * Process a structured log template message
   * @param fmt_template a structured log template message containing named arguments
   * @return first: fmt string without the named arguments, second: a vector extracted keys
   */
  QUILL_ATTRIBUTE_HOT static std::pair<std::string, std::vector<std::string>> _process_structured_log_template(
    std::string_view fmt_template) noexcept;

  /**
   * Helper function to read the unbounded queue and also report the allocation
   * @param queue queue
   * @param thread_context thread context
   * @return start position of read
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::byte* _read_unbounded_queue(UnboundedQueue& queue,
                                                                              ThreadContext* thread_context);

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline bool _check_all_queues_empty(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts);

  /**
   * Resyncs the rdtsc clock
   */
  QUILL_ATTRIBUTE_HOT void _resync_rdtsc_clock();

private:
  Config const& _config;
  ThreadContextCollection& _thread_context_collection;
  HandlerCollection& _handler_collection;
  LoggerCollection& _logger_collection;

  std::thread _backend_worker_thread; /** the backend thread that is writing the log to the handlers */

  std::atomic<RdtscClock*> _rdtsc_clock{nullptr}; /** rdtsc clock if enabled **/

  std::chrono::nanoseconds _backend_thread_sleep_duration; /** backend_thread_sleep_duration from config **/
  size_t _transit_events_soft_limit; /** limit of transit events before start flushing, value from config */
  size_t _thread_transit_events_hard_limit; /** limit for the transit event buffer value from config */

  std::vector<fmtquill::basic_format_arg<fmtquill::format_context>> _args; /** Format args tmp storage as member to avoid reallocation */
  std::vector<fmtquill::basic_format_arg<fmtquill::printf_context>> _printf_args; /** Format args tmp storage as member to avoid reallocation */

  BacktraceStorage _backtrace_log_message_storage; /** Stores a vector of backtrace messages per logger name */
  std::unordered_map<std::string, std::pair<std::string, std::vector<std::string>>> _slog_templates; /** Avoid re-formating the same structured template each time */

  /** Id of the current running process **/
  std::string _process_id;
  std::string _structured_fmt_str; /** to avoid allocation each time **/
  std::chrono::milliseconds _rdtsc_resync_interval;
  std::chrono::system_clock::time_point _last_rdtsc_resync;
  uint32_t _backend_worker_thread_id{0}; /** cached backend worker thread id */

  backend_worker_notification_handler_t _notification_handler; /** error handler for the backend thread */

  bool _backend_thread_yield; /** backend_thread_yield from config **/
  bool _has_unflushed_messages{false}; /** There are messages that are buffered by the OS, but not yet flushed */
  bool _strict_log_timestamp_order{true};
  bool _empty_all_queues_before_exit{true};
  bool _use_transit_buffer{true};
  std::atomic<bool> _is_running{false}; /** The spawned backend thread status */

  alignas(CACHE_LINE_ALIGNED) std::mutex _wake_up_mutex;
  std::condition_variable _wake_up_cv;
  bool _wake_up{false};
};

/***/
bool BackendWorker::is_running() const noexcept
{
  return _is_running.load(std::memory_order_relaxed);
}

/***/
QUILL_NODISCARD uint64_t BackendWorker::time_since_epoch(uint64_t rdtsc_value) const
{
  if (QUILL_UNLIKELY(_backend_thread_sleep_duration > _rdtsc_resync_interval))
  {
    QUILL_THROW(
      QuillError{"Invalid config, When TSC clock is used backend_thread_sleep_duration should "
                 "not be higher than rdtsc_resync_interval"});
  }

  RdtscClock const* rdtsc_clock = _rdtsc_clock.load(std::memory_order_acquire);
  return rdtsc_clock ? rdtsc_clock->time_since_epoch_safe(rdtsc_value) : 0;
}

/***/
void BackendWorker::run()
{
  // We store the configuration here on our local variables since the config flag is not atomic,
  // and we don't want it to change after we have started - This is just for safety and to
  // enforce the user to configure a variable before the thread has started
  _backend_thread_sleep_duration = _config.backend_thread_sleep_duration;
  _backend_thread_yield = _config.backend_thread_yield;
  _transit_events_soft_limit = _config.backend_thread_transit_events_soft_limit;
  _thread_transit_events_hard_limit = _config.backend_thread_transit_events_hard_limit;
  _empty_all_queues_before_exit = _config.backend_thread_empty_all_queues_before_exit;
  _strict_log_timestamp_order = _config.backend_thread_strict_log_timestamp_order;
  _rdtsc_resync_interval = _config.rdtsc_resync_interval;
  _use_transit_buffer = _config.backend_thread_use_transit_buffer;

  if (_config.backend_thread_notification_handler)
  {
    // set up the default error handler
    _notification_handler = _config.backend_thread_notification_handler;
  }

  assert(_notification_handler && "_notification_handler is always set");

  std::thread worker(
    [this]()
    {
      QUILL_TRY
      {
        if (_config.backend_thread_cpu_affinity != (std::numeric_limits<uint16_t>::max)())
        {
          // Set cpu affinity if requested to cpu _backend_thread_cpu_affinity
          set_cpu_affinity(_config.backend_thread_cpu_affinity);
        }
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _notification_handler(e.what()); }
      QUILL_CATCH_ALL() { _notification_handler(std::string{"Caught unhandled exception."}); }
#endif

      QUILL_TRY
      {
        // Set the thread name to the desired name
        set_thread_name(_config.backend_thread_name.data());
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _notification_handler(e.what()); }
      QUILL_CATCH_ALL() { _notification_handler(std::string{"Caught unhandled exception."}); }
#endif

      // Cache this thread's id
      _backend_worker_thread_id = get_thread_id();

      // All okay, set the backend worker thread running flag
      _is_running.store(true, std::memory_order_seq_cst);

      // Running
      while (QUILL_LIKELY(_is_running.load(std::memory_order_relaxed)))
      {
        // main loop
        QUILL_TRY { _main_loop(); }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e) { _notification_handler(e.what()); }
        QUILL_CATCH_ALL()
        {
          _notification_handler(std::string{"Caught unhandled exception."});
        } // clang-format on
#endif
      }

      // exit
      QUILL_TRY { _exit(); }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _notification_handler(e.what()); }
      QUILL_CATCH_ALL()
      {
        _notification_handler(std::string{"Caught unhandled exception."});
      } // clang-format on
#endif
    });

  // Move the worker ownership to our class
  _backend_worker_thread.swap(worker);

  while (!_is_running.load(std::memory_order_seq_cst))
  {
    // wait for the thread to start
    std::this_thread::sleep_for(std::chrono::microseconds{100});
  }
}

/***/
std::pair<size_t, size_t> BackendWorker::_populate_transit_event_buffer(
  ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts)
{
  uint64_t const ts_now = _strict_log_timestamp_order
    ? static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count())
    : 0;

  size_t total_events{0};
  size_t max_events{0};

  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    std::visit(
      [&total_events, &max_events, &thread_context, &ts_now, this](auto& queue)
      {
        using T = std::decay_t<decltype(queue)>;
        if constexpr ((std::is_same_v<T, UnboundedQueue>) || (std::is_same_v<T, BoundedQueue>))
        {
          // copy everything from the SPSC queue to the transit event buffer to process it later
          uint32_t const events = _read_queue_messages_and_decode(queue, thread_context, ts_now);
          total_events += events;

          if (events > max_events)
          {
            max_events = events;
          }
        }
      },
      thread_context->spsc_queue_variant());
  }

  return std::make_pair(total_events, max_events);
}

/***/
template <typename QueueT>
uint32_t BackendWorker::_read_queue_messages_and_decode(QueueT& queue, ThreadContext* thread_context, uint64_t ts_now)
{
  // Note: The producer will commit to this queue when one complete message is written.
  // This means that if we can read something from the queue it will be a full message
  // The producer will add items to the buffer :
  // |timestamp|metadata*|logger_details*|args...|
  detail::UnboundedTransitEventBuffer& transit_event_buffer = thread_context->transit_event_buffer();

  size_t const queue_capacity = queue.capacity();
  uint32_t total_bytes_read{0};

  std::byte* read_pos;
  if constexpr (std::is_same_v<QueueT, UnboundedQueue>)
  {
    read_pos = _read_unbounded_queue(queue, thread_context);
  }
  else
  {
    read_pos = queue.prepare_read();
  }

  // read max of one full queue and also max_transit events otherwise we can get stuck here forever
  // if the producer keeps producing
  while ((total_bytes_read < queue_capacity) && read_pos)
  {
    if (transit_event_buffer.size() == _thread_transit_events_hard_limit)
    {
      // stop reading the queue, we reached the transit buffer hard limit
      return transit_event_buffer.size();
    }

    std::byte* const read_begin = read_pos;

    bool res = _get_transit_event_from_queue(read_pos, thread_context, ts_now);

    if (!res)
    {
      // if _get_transit_event_from_queue returns false we stop reading
      return transit_event_buffer.size();
    }

    // Finish reading
    assert((read_pos >= read_begin) && "read_buffer should be greater or equal to read_begin");
    queue.finish_read(static_cast<uint32_t>(read_pos - read_begin));
    total_bytes_read += static_cast<uint32_t>(read_pos - read_begin);

    // read again
    if constexpr (std::is_same_v<QueueT, UnboundedQueue>)
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

  return transit_event_buffer.size();
}

/***/
bool BackendWorker::_get_transit_event_from_queue(std::byte*& read_pos, ThreadContext* thread_context, uint64_t ts_now)
{
  // First we want to allocate a new TransitEvent or use an existing one
  // to store the message from the queue
  detail::UnboundedTransitEventBuffer& transit_event_buffer = thread_context->transit_event_buffer();
  TransitEvent* transit_event = transit_event_buffer.back();
  transit_event->thread_id = thread_context->thread_id();
  transit_event->thread_name = thread_context->thread_name();

  // read the header first, and take copy of the header
  read_pos = detail::align_pointer<alignof(Header), std::byte>(read_pos);
  transit_event->header = *(reinterpret_cast<detail::Header*>(read_pos));
  read_pos += sizeof(detail::Header);

  // if we are using rdtsc clock then here we will convert the value to nanoseconds since epoch
  // doing the conversion here ensures that every transit that is inserted in the transit buffer
  // below has a header timestamp of nanoseconds since epoch and makes it even possible to
  // have Logger objects using different clocks
  if (transit_event->header.logger_details->timestamp_clock_type() == TimestampClockType::Tsc)
  {
    if (!_rdtsc_clock.load(std::memory_order_relaxed))
    {
      // Here we lazy initialise rdtsc clock on the backend thread only if the user decides to use it
      // Use rdtsc clock based on config. The clock requires a few seconds to init as it is
      // taking samples first
      _rdtsc_clock.store(new RdtscClock{_rdtsc_resync_interval}, std::memory_order_release);
      _last_rdtsc_resync = std::chrono::system_clock::now();
    }

    // convert the rdtsc value to nanoseconds since epoch
    transit_event->header.timestamp =
      _rdtsc_clock.load(std::memory_order_relaxed)->time_since_epoch(transit_event->header.timestamp);

    // Now check if the message has a timestamp greater than our ts_now
    if QUILL_UNLIKELY ((ts_now != 0) && ((transit_event->header.timestamp / 1'000) >= ts_now))
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
  else if (transit_event->header.logger_details->timestamp_clock_type() == TimestampClockType::System)
  {
    if QUILL_UNLIKELY ((ts_now != 0) && ((transit_event->header.timestamp / 1'000) >= ts_now))
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
  else if (transit_event->header.logger_details->timestamp_clock_type() == TimestampClockType::Custom)
  {
    // we skip checking against `ts_now`, we can not compare a custom timestamp by
    // the user (TimestampClockType::Custom) against ours
  }

  // we need to check and do not try to format the flush events as that wouldn't be valid
  auto const [macro_metadata, format_fns] = transit_event->header.metadata_and_format_fn();
  auto const [format_to_fn, printf_format_to_fn] = format_fns;

  if (macro_metadata.event() != MacroMetadata::Event::Flush)
  {
#if defined(_WIN32)
    if (macro_metadata.has_wide_char())
    {
      // convert the format string to a narrow string
      size_t const size_needed = get_wide_string_encoding_size(macro_metadata.wmessage_format());
      std::string format_str(size_needed, 0);
      wide_string_to_narrow(format_str.data(), size_needed, macro_metadata.wmessage_format());

      assert(!macro_metadata.is_structured_log_template() &&
             "structured log templates are not supported for wide characters");

      auto const [pos, error] = format_to_fn(format_str, read_pos, transit_event->formatted_msg, _args);
      read_pos = pos;

      if (QUILL_UNLIKELY(!error.empty()))
      {
        // this means that fmt::format_to threw an exception, and we report it to the user
        _notification_handler(fmtquill::format("Quill ERROR: {}", error));
      }
    }
    else
    {
#endif
      if (macro_metadata.is_structured_log_template())
      {
        assert(format_to_fn &&
               "format_to_fn must be set for structured log templates, printf format is not "
               "support for structured log templates");

        // using the message_format as key for lookups
        _structured_fmt_str.assign(macro_metadata.message_format().data(),
                                   macro_metadata.message_format().size());

        std::vector<std::string> const* s_keys{nullptr};

        // for messages containing named arguments threat them as structured logs
        auto const search = _slog_templates.find(_structured_fmt_str);
        if (search != std::cend(_slog_templates))
        {
          auto const& [fmt_str, structured_keys] = search->second;
          s_keys = &structured_keys;

          auto const [pos, error] = format_to_fn(fmt_str, read_pos, transit_event->formatted_msg, _args);

          read_pos = pos;

          if (QUILL_UNLIKELY(!error.empty()))
          {
            // this means that fmt::format_to threw an exception, and we report it to the user
            _notification_handler(fmtquill::format("Quill ERROR: {}", error));
          }
        }
        else
        {
          auto [fmt_str, structured_keys] =
            _process_structured_log_template(macro_metadata.message_format());

          // insert the results
          auto res = _slog_templates.try_emplace(
            _structured_fmt_str, std::make_pair(fmt_str, std::move(structured_keys)));
          s_keys = &(res.first->second.second);

          auto const [pos, error] = format_to_fn(fmt_str, read_pos, transit_event->formatted_msg, _args);

          read_pos = pos;

          if (QUILL_UNLIKELY(!error.empty()))
          {
            // this means that fmt::format_to threw an exception, and we report it to the user
            _notification_handler(fmtquill::format("Quill ERROR: {}", error));
          }
        }

        // format the values to strings
        std::vector<std::string> structured_values;
        structured_values.reserve(s_keys->size());
        for (auto const& arg : _args)
        {
          structured_values.emplace_back(fmtquill::vformat("{}", fmtquill::basic_format_args(&arg, 1)));
        }

        // store them as kv pair
        transit_event->structured_kvs.clear();
        for (size_t i = 0; i < s_keys->size(); ++i)
        {
          transit_event->structured_kvs.emplace_back((*s_keys)[i], std::move(structured_values[i]));
        }
      }
      else
      {
        // regular logs
        if (format_to_fn)
        {
          // fmt style format
          auto const [pos, error] =
            format_to_fn(macro_metadata.message_format(), read_pos, transit_event->formatted_msg, _args);

          read_pos = pos;

          if (QUILL_UNLIKELY(!error.empty()))
          {
            // this means that fmt::format_to threw an exception, and we report it to the user
            _notification_handler(fmtquill::format("Quill ERROR: {}", error));
          }
        }
        else
        {
          // printf style format
          auto const [pos, error] = printf_format_to_fn(macro_metadata.message_format(), read_pos,
                                                        transit_event->formatted_msg, _printf_args);

          read_pos = pos;

          if (QUILL_UNLIKELY(!error.empty()))
          {
            // this means that fmt::format_to threw an exception, and we report it to the user
            _notification_handler(fmtquill::format("Quill ERROR: {}", error));
          }
        }
      }

      if (macro_metadata.level() == LogLevel::Dynamic)
      {
        // if this is a dynamic log level we need to read the log level from the buffer
        LogLevel dynamic_log_level;
        std::memcpy(&dynamic_log_level, read_pos, sizeof(LogLevel));
        read_pos += sizeof(LogLevel);

        // Also set the dynamic log level to the transit event
        transit_event->log_level_override = dynamic_log_level;
      }
#if defined(_WIN32)
    }
#endif
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
  transit_event_buffer.push_back();

  return true;
}

/***/
void BackendWorker::_process_transit_events(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts)
{
  // Get the lowest timestamp
  uint64_t min_ts{std::numeric_limits<uint64_t>::max()};
  detail::UnboundedTransitEventBuffer* transit_buffer{nullptr};

  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    TransitEvent* te = thread_context->transit_event_buffer().front();
    if (te && min_ts > te->header.timestamp)
    {
      min_ts = te->header.timestamp;
      transit_buffer = std::addressof(thread_context->transit_event_buffer());
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

  _process_transit_event(*transit_event);

  // Remove this event and move to the next.
  transit_buffer->pop_front();
}

/***/
void BackendWorker::_process_transit_event(TransitEvent& transit_event)
{
  MacroMetadata const macro_metadata = transit_event.metadata();

  // If backend_process(...) throws we want to skip this event and move to the next, so we catch the
  // error here instead of catching it in the parent try/catch block of main_loop
  QUILL_TRY
  {
    if (macro_metadata.event() == MacroMetadata::Event::Log)
    {
      if (transit_event.log_level() != LogLevel::Backtrace)
      {
        _write_transit_event(transit_event);

        // We also need to check the severity of the log message here against the backtrace
        // Check if we should also flush the backtrace messages:
        // After we forwarded the message we will check the severity of this message for this logger
        // If the severity of the message is higher than the backtrace flush severity we will also
        // flush the backtrace of the logger
        if (QUILL_UNLIKELY(transit_event.log_level() >= transit_event.header.logger_details->backtrace_flush_level()))
        {
          _backtrace_log_message_storage.process(transit_event.header.logger_details->name(),
                                                 [this](TransitEvent const& transit_event)
                                                 { _write_transit_event(transit_event); });
        }
      }
      else
      {
        // this is a backtrace log and we will store it
        _backtrace_log_message_storage.store(std::move(transit_event));
      }
    }
    else if (macro_metadata.event() == MacroMetadata::Event::InitBacktrace)
    {
      // we can just convert the capacity back to int here and use it
      _backtrace_log_message_storage.set_capacity(
        transit_event.header.logger_details->name(),
        static_cast<uint32_t>(std::stoul(
          std::string{transit_event.formatted_msg.begin(), transit_event.formatted_msg.end()})));
    }
    else if (macro_metadata.event() == MacroMetadata::Event::FlushBacktrace)
    {
      // process all records in backtrace for this logger_name and log them by calling backend_process_backtrace_log_message
      _backtrace_log_message_storage.process(transit_event.header.logger_details->name(),
                                             [this](TransitEvent const& transit_event)
                                             { _write_transit_event(transit_event); });
    }
    else if (macro_metadata.event() == MacroMetadata::Event::Flush)
    {
      _force_flush(_handler_collection.active_handlers());

      // this is a flush event, so we need to notify the caller to continue now
      transit_event.flush_flag->store(true);

      // we also need to reset the flush_flag as the TransitEvents are re-used
      transit_event.flush_flag = nullptr;
    }

    // Since after processing an event we never force flush but leave it up to the OS instead,
    // set this to true to keep track of unflushed messages we have
    _has_unflushed_messages = true;
  }
#if !defined(QUILL_NO_EXCEPTIONS)
  QUILL_CATCH(std::exception const& e) { _notification_handler(e.what()); }
  QUILL_CATCH_ALL()
  {
    _notification_handler(std::string{"Caught unhandled exception."});
  } // clang-format on
#endif
}

/***/
void BackendWorker::_write_transit_event(TransitEvent const& transit_event)
{
  // Forward the record to all the logger handlers
  MacroMetadata const macro_metadata = transit_event.metadata();

  for (auto& handler : transit_event.header.logger_details->handlers())
  {
    auto const& formatted_log_message_buffer = handler->formatter().format(
      std::chrono::nanoseconds{transit_event.header.timestamp}, transit_event.thread_id,
      transit_event.thread_name, _process_id, transit_event.header.logger_details->name(),
      transit_event.log_level_as_str(), macro_metadata, transit_event.formatted_msg);

    // If all filters are okay we write this message to the file
    if (handler->apply_filters(transit_event.thread_id,
                               std::chrono::nanoseconds{transit_event.header.timestamp},
                               transit_event.log_level(), macro_metadata, formatted_log_message_buffer))
    {
      // log to the handler, also pass the log_message_timestamp this is only needed in some
      // cases like daily file rotation
      handler->write(formatted_log_message_buffer, transit_event);
    }
  }
}

/***/
bool BackendWorker::_process_and_write_single_message(const ThreadContextCollection::backend_thread_contexts_cache_t& cached_thread_contexts)
{
  ThreadContext* tc{nullptr};
  uint64_t min_ts{std::numeric_limits<uint64_t>::max()};

  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    std::visit(
      [&thread_context, &min_ts, &tc, this](auto& queue)
      {
        // find the minimum timestamp accross all queues
        using T = std::decay_t<decltype(queue)>;
        if constexpr ((std::is_same_v<T, UnboundedQueue>) || (std::is_same_v<T, BoundedQueue>))
        {
          std::byte* read_pos;
          if constexpr (std::is_same_v<T, UnboundedQueue>)
          {
            read_pos = _read_unbounded_queue(queue, thread_context);
          }
          else
          {
            read_pos = queue.prepare_read();
          }

          if (read_pos && (reinterpret_cast<detail::Header*>(read_pos)->timestamp < min_ts))
          {
            min_ts = reinterpret_cast<detail::Header*>(read_pos)->timestamp;
            tc = thread_context;
          }
        }
      },
      thread_context->spsc_queue_variant());
  }

  if (!tc)
  {
    // all queues are empty
    return false;
  }

  std::visit(
    [this, &tc](auto& queue)
    {
      using T = std::decay_t<decltype(queue)>;
      if constexpr ((std::is_same_v<T, UnboundedQueue>) || (std::is_same_v<T, BoundedQueue>))
      {
        std::byte* read_pos;
        if constexpr (std::is_same_v<T, UnboundedQueue>)
        {
          read_pos = _read_unbounded_queue(queue, tc);
        }
        else
        {
          read_pos = queue.prepare_read();
        }
        assert(read_pos);

        std::byte* const read_begin = read_pos;

        _get_transit_event_from_queue(read_pos, tc, 0);

        // Finish reading
        assert((read_pos >= read_begin) && "read_buffer should be greater or equal to read_begin");
        queue.finish_read(static_cast<uint32_t>(read_pos - read_begin));
        queue.commit_read();
      }
    },
    tc->spsc_queue_variant());

  return true;
}

/***/
void BackendWorker::_force_flush(std::vector<std::weak_ptr<Handler>> const& active_handlers)
{
  if (_has_unflushed_messages)
  {
    // If we have buffered any messages then flush all active handlers
    for (auto const& handler : active_handlers)
    {
      std::shared_ptr<Handler> h = handler.lock();
      if (h)
      {
        h->flush();
      }
    }

    _has_unflushed_messages = false;
  }
}

/***/
void BackendWorker::_check_message_failures(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts,
                                            backend_worker_notification_handler_t const& notification_handler) noexcept
{
  if constexpr (QUILL_QUEUE_TYPE == detail::QueueType::UnboundedNoMaxLimit)
  {
    // UnboundedNoMaxLimit does not block or drop messages
    return;
  }

  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    size_t const failed_messages_cnt = thread_context->get_and_reset_message_failure_counter();

    if (QUILL_UNLIKELY(failed_messages_cnt > 0))
    {
      char ts[24];
      time_t t = time(nullptr);
      struct tm p;
      quill::detail::localtime_rs(std::addressof(t), std::addressof(p));
      strftime(ts, 24, "%X", std::addressof(p));

      if constexpr (QUILL_QUEUE_TYPE == detail::QueueType::BoundedNonBlocking)
      {
        notification_handler(
          fmtquill::format("{} Quill INFO: BoundedNonBlocking queue dropped {} "
                           "log messages from thread {}",
                           ts, failed_messages_cnt, thread_context->thread_id()));
      }
      else if constexpr (QUILL_QUEUE_TYPE == detail::QueueType::UnboundedDropping)
      {
        notification_handler(
          fmtquill::format("{} Quill INFO: UnboundedDropping queue dropped {} "
                           "log messages from thread {}",
                           ts, failed_messages_cnt, thread_context->thread_id()));
      }
      else if constexpr (QUILL_QUEUE_TYPE == detail::QueueType::BoundedBlocking)
      {
        notification_handler(
          fmtquill::format("{} Quill INFO: BoundedBlocking queue thread {} "
                           "experienced {} blocking occurrences",
                           ts, thread_context->thread_id(), failed_messages_cnt));
      }
      else if constexpr (QUILL_QUEUE_TYPE == detail::QueueType::UnboundedBlocking)
      {
        notification_handler(
          fmtquill::format("{} Quill INFO: UnboundedBlocking queue thread {} "
                           "experienced {} blocking occurrences",
                           ts, thread_context->thread_id(), failed_messages_cnt));
      }
    }
  }
}

/***/
std::byte* BackendWorker::_read_unbounded_queue(UnboundedQueue& queue, ThreadContext* thread_context)
{
  auto [read_pos, allocation_info] = queue.prepare_read();

  if (allocation_info)
  {
    // When allocation_info has a value it means that the queue has re-allocated
    if (_notification_handler)
    {
      char ts[24];
      time_t t = time(nullptr);
      struct tm p;
      quill::detail::localtime_rs(std::addressof(t), std::addressof(p));
      strftime(ts, 24, "%X", std::addressof(p));

      // we switched to a new here, and we also notify the user of the allocation via the
      // notification_handler
      _notification_handler(fmtquill::format(
        "{} Quill INFO: A new SPSC queue has been allocated with a new capacity of {} bytes and "
        "a previous capacity of {} bytes from thread {}",
        ts, allocation_info->first, allocation_info->second, thread_context->thread_id()));
    }
  }

  return read_pos;
}

/***/

bool BackendWorker::_check_all_queues_empty(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts)
{
  bool all_empty{true};

  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    std::visit(
      [&all_empty](auto& queue)
      {
        using T = std::decay_t<decltype(queue)>;
        if constexpr ((std::is_same_v<T, UnboundedQueue>) || (std::is_same_v<T, BoundedQueue>))
        {
          all_empty &= queue.empty();
        }
      },
      thread_context->spsc_queue_variant());
  }

  return all_empty;
}

/***/
void BackendWorker::_main_loop()
{
  // load all contexts locally
  ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  size_t total_events{0};

  if (_use_transit_buffer)
  {
    auto const [tevents, max_events] = _populate_transit_event_buffer(cached_thread_contexts);
    total_events = tevents;

    if ((total_events != 0))
    {
      // there are buffered events to process
      if (total_events >= _transit_events_soft_limit)
      {
        // we can log only up to max_events, then we want to re-read the queue to avoid
        // logging out of order messages
        for (size_t i = 0; i < (max_events - 1); ++i)
        {
          _process_transit_events(cached_thread_contexts);
        }
      }
      else
      {
        // process a single transit event, then give priority to the hot thread spsc queue again
        _process_transit_events(cached_thread_contexts);
      }
    }
  }
  else
  {
    bool const res = _process_and_write_single_message(cached_thread_contexts);
    if (res)
    {
      total_events = 1;

      // process a single transit event, then give priority to the hot thread spsc queue again
      _process_transit_events(cached_thread_contexts);
    }
  }

  if (total_events == 0)
  {
    // None of the thread local queues had any events to process, this means we have processed
    // all messages in all queues We force flush all remaining messages
    std::vector<std::weak_ptr<Handler>> const active_handlers = _handler_collection.active_handlers();
    _force_flush(active_handlers);

    // invoke the Handler's periodic loop
    for (auto const& handler : active_handlers)
    {
      std::shared_ptr<Handler> h = handler.lock();
      if (h)
      {
        h->run_loop();
      }
    }

    // check for any dropped messages / blocked threads
    _check_message_failures(cached_thread_contexts, _notification_handler);

    // We can also clear any invalidated or empty thread contexts
    _thread_context_collection.clear_invalid_and_empty_thread_contexts();

    // resync rdtsc clock before going to sleep.
    // This is useful when quill::Clock is used
    _resync_rdtsc_clock();

    // Also check if all queues are empty as we need to know that to remove any unused Loggers
    bool all_queues_empty = _check_all_queues_empty(cached_thread_contexts);

    if (all_queues_empty)
    {
      // since there are no messages we can check for invalidated loggers and clean them up
      bool const loggers_removed = _logger_collection.remove_invalidated_loggers(
        [this]()
        {
          // we need to reload all thread contexts and check again for empty queues before remove a logger to avoid race condition
          return _check_all_queues_empty(_thread_context_collection.backend_thread_contexts_cache());
        });

      if (loggers_removed)
      {
        // if loggers were removed also check for Handlers to remove
        // remove_unused_handlers is expensive and should be only called when it is needed
        _handler_collection.remove_unused_handlers();
      }

      // There is nothing left to do, and we can let this thread sleep for a while
      // buffer events are 0 here and also all the producer queues are empty
      if (_backend_thread_sleep_duration.count() != 0)
      {
        std::unique_lock<std::mutex> lock(_wake_up_mutex);

        // Wait for a timeout or a notification to wake up
        _wake_up_cv.wait_for(lock, _backend_thread_sleep_duration, [this] { return _wake_up; });

        // set the flag back to false since we woke up here
        _wake_up = false;

        // After waking up resync rdtsc clock again and resume
        _resync_rdtsc_clock();
      }
      else if (_backend_thread_yield)
      {
        std::this_thread::yield();
      }
    }
  }
}

/***/
void BackendWorker::_exit()
{
  // load all contexts locally
  ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  while (true)
  {
    size_t total_events{0};

    if (_use_transit_buffer)
    {
      auto const [tevents, max_events] = _populate_transit_event_buffer(cached_thread_contexts);
      total_events = tevents;

      if ((total_events != 0))
      {
        // there are buffered events to process
        if (total_events >= _transit_events_soft_limit)
        {
          // we can log only up to max_events, then we want to re-read the queue to avoid
          // logging out of order messages
          for (size_t i = 0; i < (max_events - 1); ++i)
          {
            _process_transit_events(cached_thread_contexts);
          }
        }
        else
        {
          // process a single transit event, then give priority to the hot thread spsc queue again
          _process_transit_events(cached_thread_contexts);
        }
      }
    }
    else
    {
      bool const res = _process_and_write_single_message(cached_thread_contexts);
      if (res)
      {
        total_events = 1;

        // process a single transit event, then give priority to the hot thread spsc queue again
        _process_transit_events(cached_thread_contexts);
      }
    }

    if (total_events == 0)
    {
      bool all_empty{true};

      if (_empty_all_queues_before_exit)
      {
        all_empty = _check_all_queues_empty(cached_thread_contexts);
      }

      if (all_empty)
      {
        // we are done, all queues are now empty
        _check_message_failures(cached_thread_contexts, _notification_handler);
        _force_flush(_handler_collection.active_handlers());
        break;
      }
    }
  }

  RdtscClock* rdtsc_clock{_rdtsc_clock.load(std::memory_order_relaxed)};
  _rdtsc_clock.store(nullptr, std::memory_order_release);
  delete rdtsc_clock;
}
} // namespace quill::detail
