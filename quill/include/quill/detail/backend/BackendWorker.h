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
   * @param queue queue
   * @param thread_context thread context
   * @param ts_now timestamp now
   * @return total events stored in the transit_event_buffer
   */
  template <typename QueueT>
  QUILL_ATTRIBUTE_HOT uint32_t _read_queue_messages_and_decode(QueueT& queue, ThreadContext* thread_context,
                                                               uint64_t ts_now);

  QUILL_ATTRIBUTE_HOT bool _get_transit_event_from_queue(std::byte*& read_pos,
                                                         ThreadContext* thread_context, uint64_t ts_now);

  /**
   * Checks for events in all queues and processes the one with the minimum timestamp
   */
  QUILL_ATTRIBUTE_HOT inline void _process_transit_events(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts);

  /**
   * Process a single trnasit event
   */
  QUILL_ATTRIBUTE_HOT void _process_transit_event(TransitEvent& transit_event);

  /**
   * Write a transit event
   */
  QUILL_ATTRIBUTE_HOT void _write_transit_event(TransitEvent const& transit_event) const;

  /**
   * Process the lowest timestamp from the queues and write it to the log file
   */
  QUILL_ATTRIBUTE_HOT inline bool _process_and_write_single_message(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts);

  /**
   * Check for dropped messages - only when bounded queue is used
   * @param cached_thread_contexts loaded thread contexts
   * @param notification_handler notification handler
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
                                                                              ThreadContext* thread_context) const;

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool _check_all_queues_empty(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts);

  /**
   * Resyncs the rdtsc clock
   */
  QUILL_ATTRIBUTE_HOT void _resync_rdtsc_clock();

  /**
   * Updates the active handlers cache
   */
  void _flush_and_run_active_handlers_loop(bool run_loop)
  {
    if (_logger_collection.has_invalidated_loggers())
    {
      // If there are invalidated loggers we take a slower path and exclude the handlers of
      // the invalidated loggers
      _logger_collection.active_handlers(_active_handlers_cache);
    }
    else
    {
      _handler_collection.active_handlers(_active_handlers_cache);
    }

    for (auto const& handler : _active_handlers_cache)
    {
      std::shared_ptr<Handler> h = handler.lock();
      if (h)
      {
        QUILL_TRY
        {
          // If an exception is thrown, catch it here to prevent it from propagating
          // to the outer function. This prevents potential infinite loops caused by failing flush operations.
          h->flush();
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e) { _notification_handler(e.what()); }
        QUILL_CATCH_ALL() { _notification_handler(std::string{"Caught unhandled exception."}); }
#endif

        if (run_loop)
        {
          h->run_loop();
        }
      }
    }
  }

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
  std::vector<std::weak_ptr<Handler>> _active_handlers_cache;

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
  UnboundedTransitEventBuffer const& transit_event_buffer = thread_context->transit_event_buffer();

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

    std::byte const* const read_begin = read_pos;

    if (!_get_transit_event_from_queue(read_pos, thread_context, ts_now))
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
void BackendWorker::_process_transit_events(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts)
{
  // Get the lowest timestamp
  uint64_t min_ts{std::numeric_limits<uint64_t>::max()};
  UnboundedTransitEventBuffer* transit_buffer{nullptr};

  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    if (TransitEvent const* te = thread_context->transit_event_buffer().front(); te && (min_ts > te->timestamp))
    {
      min_ts = te->timestamp;
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

  QUILL_TRY { _process_transit_event(*transit_event); }
#if !defined(QUILL_NO_EXCEPTIONS)
  QUILL_CATCH(std::exception const& e) { _notification_handler(e.what()); }
  QUILL_CATCH_ALL()
  {
    _notification_handler(std::string{"Caught unhandled exception."});
  } // clang-format on
#endif

  // Remove this event and move to the next.
  transit_buffer->pop_front();
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

          if (read_pos)
          {
            read_pos = align_pointer<alignof(uint64_t), std::byte>(read_pos);
            uint64_t timestamp;
            std::memcpy(&timestamp, read_pos, sizeof(uint64_t));

            if (timestamp < min_ts)
            {
              min_ts = timestamp;
              tc = thread_context;
            }
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

        std::byte const* const read_begin = read_pos;

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
void BackendWorker::_check_message_failures(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts,
                                            backend_worker_notification_handler_t const& notification_handler) noexcept
{
  // UnboundedNoMaxLimit does not block or drop messages
  if constexpr (QUILL_QUEUE_TYPE != QueueType::UnboundedNoMaxLimit)
  {
    for (ThreadContext* thread_context : cached_thread_contexts)
    {
      size_t const failed_messages_cnt = thread_context->get_and_reset_message_failure_counter();

      if (QUILL_UNLIKELY(failed_messages_cnt > 0))
      {
        char ts[24];
        time_t t = time(nullptr);
        tm p;
        localtime_rs(std::addressof(t), std::addressof(p));
        strftime(ts, 24, "%X", std::addressof(p));

        if constexpr (QUILL_QUEUE_TYPE == QueueType::BoundedNonBlocking)
        {
          notification_handler(
            fmtquill::format("{} Quill INFO: BoundedNonBlocking queue dropped {} "
                             "log messages from thread {}",
                             ts, failed_messages_cnt, thread_context->thread_id()));
        }
        else if constexpr (QUILL_QUEUE_TYPE == QueueType::UnboundedDropping)
        {
          notification_handler(
            fmtquill::format("{} Quill INFO: UnboundedDropping queue dropped {} "
                             "log messages from thread {}",
                             ts, failed_messages_cnt, thread_context->thread_id()));
        }
        else if constexpr (QUILL_QUEUE_TYPE == QueueType::BoundedBlocking)
        {
          notification_handler(
            fmtquill::format("{} Quill INFO: BoundedBlocking queue thread {} "
                             "experienced {} blocking occurrences",
                             ts, thread_context->thread_id(), failed_messages_cnt));
        }
        else if constexpr (QUILL_QUEUE_TYPE == QueueType::UnboundedBlocking)
        {
          notification_handler(
            fmtquill::format("{} Quill INFO: UnboundedBlocking queue thread {} "
                             "experienced {} blocking occurrences",
                             ts, thread_context->thread_id(), failed_messages_cnt));
        }
      }
    }
  }
}

/***/
std::byte* BackendWorker::_read_unbounded_queue(UnboundedQueue& queue, ThreadContext* thread_context) const
{
  auto [read_pos, allocation_info] = queue.prepare_read();

  if (allocation_info)
  {
    // When allocation_info has a value it means that the queue has re-allocated
    if (_notification_handler)
    {
      char ts[24];
      time_t t = time(nullptr);
      tm p;
      localtime_rs(std::addressof(t), std::addressof(p));
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

    _flush_and_run_active_handlers_loop(true);

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
        std::unique_lock<std::mutex> lock{_wake_up_mutex};

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
        _flush_and_run_active_handlers_loop(false);
        break;
      }
    }
  }

  RdtscClock const* rdtsc_clock{_rdtsc_clock.load(std::memory_order_relaxed)};
  _rdtsc_clock.store(nullptr, std::memory_order_release);
  delete rdtsc_clock;
}
} // namespace quill::detail
