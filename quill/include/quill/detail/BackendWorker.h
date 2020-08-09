/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/QuillError.h"                       // for QUILL_CATCH, QUILL...
#include "quill/detail/BacktraceLogRecordStorage.h" // for BacktraceLogRecordStorage
#include "quill/detail/BoundedSPSCQueue.h"          // for BoundedSPSCQueue<>...
#include "quill/detail/Config.h"                    // for Config
#include "quill/detail/HandlerCollection.h"         // for HandlerCollection
#include "quill/detail/ThreadContext.h"             // for ThreadContext, Thr...
#include "quill/detail/ThreadContextCollection.h"   // for ThreadContextColle...
#include "quill/detail/events/BaseEvent.h"          // for RecordBase
#include "quill/detail/misc/Attributes.h"           // for QUILL_ATTRIBUTE_HOT
#include "quill/detail/misc/Common.h"               // for QUILL_RDTSC_RESYNC...
#include "quill/detail/misc/Macros.h"               // for QUILL_LIKELY
#include "quill/detail/misc/Os.h"                   // for set_cpu_affinity, get_thread_id
#include "quill/detail/misc/RdtscClock.h"           // for RdtscClock
#include "quill/handlers/Handler.h"               // for Handler
#include <atomic>                                 // for atomic, memory_ord...
#include <cassert>                                // for assert
#include <chrono>                                 // for nanoseconds, milli...
#include <cstdint>                                // for uint16_t
#include <exception>                              // for exception
#include <functional>                             // for greater, function
#include <limits>                                 // for numeric_limits
#include <memory>                                 // for unique_ptr, make_u...
#include <mutex>                                  // for call_once, once_flag
#include <queue>                                  // for priority_queue
#include <string>                                 // for allocator, string
#include <thread>                                 // for sleep_for, thread
#include <utility>                                // for move
#include <vector>                                 // for vector

namespace quill
{
using backend_worker_error_handler_t = std::function<void(std::string const&)>;

namespace detail
{

class BackendWorker
{
public:
  /**
   * Constructor
   */
  BackendWorker(Config const& config, ThreadContextCollection& thread_context_collection,
                HandlerCollection const& handler_collection);

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

#if !defined(QUILL_NO_EXCEPTIONS)
  /**
   * Set up a custom error handler that will be used if the backend thread has any error.
   * If no error handler is set, the default one will print to std::cerr
   * @param error_handler an error handler callback e.g [](std::string const& s) { std::cerr << s << std::endl; }
   * @throws exception if it is called after the thread has started
   */
  QUILL_ATTRIBUTE_COLD void set_error_handler(backend_worker_error_handler_t error_handler);
#endif

private:
  /**
   * Backend worker thread main function
   */
  QUILL_ATTRIBUTE_HOT inline void _main_loop();

  /**
   * Logging thread exist function that flushes everything after stop() is called
   */
  QUILL_ATTRIBUTE_COLD inline void _exit();

  /**
   * Populate our local priority queue
   * @param cached_thread_contexts local thread context cache
   */
  QUILL_ATTRIBUTE_HOT inline void _populate_priority_queue(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts);

  /**
   * Checks for events in all queues and processes the one with the minimum timestamp
   */
  QUILL_ATTRIBUTE_HOT inline void _process_event();

  /**
   * Force flush all active Handlers
   */
  QUILL_ATTRIBUTE_HOT inline void _force_flush();

  /**
   * Convert a timestamp from BaseEvent to a time since epoch timestamp in nanoseconds.
   *
   * @param base_event The base event timestamp is just an uint64 and it can be either
   * rdtsc time or nanoseconds since epoch based on #if !defined(QUILL_CHRONO_CLOCK) definition
   * @return a timestamp in nanoseconds since epoch
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::chrono::nanoseconds _get_real_timestamp(BaseEvent const* base_event) const noexcept;

  /**
   * Check for dropped messages - only when bounded queue is used
   * @param cached_thread_contexts loaded thread contexts
   */
  QUILL_ATTRIBUTE_HOT static void _check_dropped_messages(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts) noexcept;

private:
  struct TransitEvent
  {
    TransitEvent(ThreadContext* in_thread_context, std::unique_ptr<BaseEvent> base_event)
      : thread_context(in_thread_context), base_event(std::move(base_event))
    {
    }

    friend bool operator>(TransitEvent const& lhs, TransitEvent const& rhs)
    {
      return lhs.base_event->timestamp() > rhs.base_event->timestamp();
    }

    ThreadContext* thread_context; /** We clean any invalidated thread_context after the priority queue is empty, so this can not be invalid */
    std::unique_ptr<BaseEvent> base_event;
  };

private:
  Config const& _config;
  ThreadContextCollection& _thread_context_collection;
  HandlerCollection const& _handler_collection;

  std::thread _backend_worker_thread; /** the backend thread that is writing the log to the handlers */
  uint32_t _backend_worker_thread_id; /** cached backend worker thread id */

  std::unique_ptr<RdtscClock> _rdtsc_clock{nullptr}; /** rdtsc clock if enabled **/

  std::chrono::nanoseconds _backend_thread_sleep_duration; /** backend_thread_sleep_duration from config **/
  std::once_flag _start_init_once_flag; /** flag to start the thread only once, in case start() is called multiple times */
  bool _has_unflushed_messages{false}; /** There are messages that are buffered by the OS, but not yet flushed */
  std::atomic<bool> _is_running{false}; /** The spawned backend thread status */
  std::priority_queue<TransitEvent, std::vector<TransitEvent>, std::greater<>> _transit_events;

  BacktraceLogRecordStorage _backtrace_log_record_storage; /** Stores a vector of backtrace log records per logger name */

#if !defined(QUILL_NO_EXCEPTIONS)
  backend_worker_error_handler_t _error_handler; /** error handler for the backend thread */
#endif
};

/***/
bool BackendWorker::is_running() const noexcept
{
  return _is_running.load(std::memory_order_relaxed);
}

/***/
void BackendWorker::run()
{
  // protect init to be called only once
  std::call_once(_start_init_once_flag, [this]() {
    // We store the configuration here on our local variable since the config flag is not atomic
    // and we don't want it to change after we have started - This is just for safety and to
    // enforce the user to configure a variable before the thread has started
    _backend_thread_sleep_duration = _config.backend_thread_sleep_duration();

    std::thread worker([this]() {
      QUILL_TRY
      {
        // On Start
        if (_config.backend_thread_cpu_affinity() != (std::numeric_limits<uint16_t>::max()))
        {
          // Set cpu affinity if requested to cpu _backend_thread_cpu_affinity
          set_cpu_affinity(_config.backend_thread_cpu_affinity());
        }

        // Set the thread name to the desired name
        set_thread_name(_config.backend_thread_name().data());
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _error_handler(e.what()); }
      QUILL_CATCH_ALL() { _error_handler(std::string{"Caught unhandled exception."}); }
#endif

#if !defined(QUILL_CHRONO_CLOCK)
      // Use rdtsc clock based on config. The clock requires a few seconds to init as it is
      // taking samples first
      _rdtsc_clock = std::make_unique<RdtscClock>(std::chrono::milliseconds{QUILL_RDTSC_RESYNC_INTERVAL});
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
        QUILL_CATCH(std::exception const& e) { _error_handler(e.what()); }
        QUILL_CATCH_ALL()
        {
          _error_handler(std::string{"Caught unhandled exception."});
        } // clang-format on
#endif
      }

      // exit
      QUILL_TRY { _exit(); }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _error_handler(e.what()); }
      QUILL_CATCH_ALL()
      {
        _error_handler(std::string{"Caught unhandled exception."});
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
  });
}

/***/
void BackendWorker::_populate_priority_queue(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts)
{
  // copy everything to a priority queue
  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    ThreadContext::SPSCQueueT& spsc_queue = thread_context->spsc_queue();

    while (true)
    {
      auto handle = spsc_queue.try_pop();

      if (!handle.is_valid())
      {
        break;
      }
      _transit_events.emplace(thread_context, handle.data()->clone());
    }
  }
}

/***/
void BackendWorker::_process_event()
{
  TransitEvent const& transit_event = _transit_events.top();

  // A lambda to obtain the logger details and pass them to backend_process(...), this lambda is
  // called only in case we need to flush because we are processing a FlushEvent
  auto obtain_active_handlers = [this]() { return _handler_collection.active_handlers(); };

  // This lambda will call our member function _get_real_timestamp
  auto get_real_ts = [this](BaseEvent const* base_event) { return _get_real_timestamp(base_event); };

  // If backend_process(...) throws we want to skip this event and move to the next so we catch the
  // error here instead of catching it in the parent try/catch block of main_loop
  QUILL_TRY
  {
    transit_event.base_event->backend_process(_backtrace_log_record_storage,
                                              transit_event.thread_context->thread_id(),
                                              obtain_active_handlers, get_real_ts);

    // Remove this event and move to the next
    _transit_events.pop();

    // Since after processing an event we never force flush but leave it up to the OS instead,
    // set this to true to keep track of unflushed messages we have
    _has_unflushed_messages = true;
  }
#if !defined(QUILL_NO_EXCEPTIONS)
  QUILL_CATCH(std::exception const& e)
  {
    _error_handler(e.what());

    // Remove this event and move to the next
    _transit_events.pop();
  }
  QUILL_CATCH_ALL()
  {
    _error_handler(std::string{"Caught unhandled exception."});

    // Remove this event and move to the next
    _transit_events.pop();
  } // clang-format on
#endif
}

void BackendWorker::_force_flush()
{
  if (_has_unflushed_messages)
  {
    // If we have buffered any messages then get all active handlers and call flush
    std::vector<Handler*> const active_handlers = _handler_collection.active_handlers();
    for (auto handler : active_handlers)
    {
      handler->flush();
    }

    _has_unflushed_messages = false;
  }
}

/***/
void BackendWorker::_main_loop()
{
  // load all contexts locally
  ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  _populate_priority_queue(cached_thread_contexts);

  if (QUILL_LIKELY(!_transit_events.empty()))
  {
    // the queue is not empty
    _process_event();
  }
  else
  {
    // there was nothing to process

    // None of the thread local queues had any events to process, this means we have processed
    // all messages in all queues We will force flush any unflushed messages and then sleep
    _force_flush();

    // check for any dropped messages by the threads
    _check_dropped_messages(cached_thread_contexts);

    // We can also clear any invalidated or empty thread contexts now that our priority queue was empty
    _thread_context_collection.clear_invalid_and_empty_thread_contexts();

    // Sleep for the specified duration as we found no events in any of the queues to process
    std::this_thread::sleep_for(_backend_thread_sleep_duration);
  }
}

/***/
std::chrono::nanoseconds BackendWorker::_get_real_timestamp(BaseEvent const* base_event) const noexcept
{
#if !defined(QUILL_CHRONO_CLOCK)

  assert(_rdtsc_clock && "rdtsc should not be nullptr");
  assert(base_event->using_rdtsc() &&
         "BaseEvent has a std::chrono timestamp, but the backend thread is using rdtsc timestamp");

  // pass to our clock the stored rdtsc from the caller thread
  return _rdtsc_clock->time_since_epoch(base_event->timestamp());
#else
  assert(!_rdtsc_clock && "rdtsc should be nullptr");
  assert(!base_event->using_rdtsc() &&
         "BaseEvent has a rdtsc clock timestamp, but the backend thread is using std::chrono "
         "timestamp");

  // Then the timestamp() will be already in epoch no need to convert it like above
  // The precision of system_clock::time-point is not portable across platforms.
  std::chrono::system_clock::duration const timestamp_duration{base_event->timestamp()};
  return std::chrono::nanoseconds{timestamp_duration};
#endif
}

/***/
void BackendWorker::_exit()
{
  // load all contexts locally
  ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  while (true)
  {
    _populate_priority_queue(cached_thread_contexts);

    if (!_transit_events.empty())
    {
      _process_event();
    }
    else
    {
      _check_dropped_messages(cached_thread_contexts);

      // keep going until there are no events are found
      break;
    }
  }
}
} // namespace detail
} // namespace quill