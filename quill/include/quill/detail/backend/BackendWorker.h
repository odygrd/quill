/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/QuillError.h"                               // for QUILL_CATCH, QUILL...
#include "quill/detail/Config.h"                            // for Config
#include "quill/detail/HandlerCollection.h"                 // for HandlerCollection
#include "quill/detail/ThreadContext.h"                     // for ThreadContext, Thr...
#include "quill/detail/ThreadContextCollection.h"           // for ThreadContextColle...
#include "quill/detail/backend/BacktraceLogRecordStorage.h" // for BacktraceLogRecordStorage
#include "quill/detail/backend/FreeListAllocator.h"         // for FreeListAllocator..
#include "quill/detail/events/BaseEvent.h"                  // for RecordBase
#include "quill/detail/misc/Attributes.h"                   // for QUILL_ATTRIBUTE_HOT
#include "quill/detail/misc/Common.h"                       // for QUILL_RDTSC_RESYNC...
#include "quill/detail/misc/Macros.h"                       // for QUILL_LIKELY
#include "quill/detail/misc/Os.h"                           // for set_cpu_affinity, get_thread_id
#include "quill/detail/misc/RdtscClock.h"                   // for RdtscClock
#include "quill/detail/misc/Utilities.h"
#include "quill/detail/serialize/Deserialize.h"
#include "quill/detail/serialize/SerializationMetadata.h"
#include "quill/detail/spsc_queue/BoundedSPSCEventQueue.h" // for BoundedSPSCQueue<>...
#include "quill/handlers/Handler.h"                        // for Handler
#include <atomic>                                          // for atomic, memory_ord...
#include <cassert>                                         // for assert
#include <chrono>                                          // for nanoseconds, milli...
#include <cstdint>                                         // for uint16_t
#include <exception>                                       // for exception
#include <functional>                                      // for greater, function
#include <limits>                                          // for numeric_limits
#include <memory>                                          // for unique_ptr, make_u...
#include <queue>                                           // for priority_queue
#include <string>                                          // for allocator, string
#include <thread>                                          // for sleep_for, thread
#include <utility>                                         // for move
#include <vector>                                          // for vector

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
   * Logging thread exit function that flushes everything after stop() is called
   */
  QUILL_ATTRIBUTE_COLD inline void _exit();

  /**
   * Populate our local priority queue
   * @param cached_thread_contexts local thread context cache
   * @param is_terminating
   */
  QUILL_ATTRIBUTE_HOT inline void _populate_priority_queue(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts, bool is_terminating);

#if !defined(QUILL_DISABLE_DUAL_QUEUE_MODE)
  /**
   * Deserialize an log message from the raw SPSC queue and emplace them to priority queue
   */
  QUILL_ATTRIBUTE_HOT inline void _deserialize_raw_queue(ThreadContext* thread_context, bool is_terminating);
#endif

  /**
   * Read events from the Event SPSC queue and emplace them to the priority queue
   */
  QUILL_ATTRIBUTE_HOT inline void _read_event_queue(ThreadContext* thread_context, bool is_terminating);

  /**
   * Checks for events in all queues and processes the one with the minimum timestamp
   */
  QUILL_ATTRIBUTE_HOT inline void _process_transit_event();

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
    /**
     * Constructor used when we are pulling event from the generic_queue
     * @param in_thread_context thread context
     * @param in_base_event the base event
     */
    TransitEvent(ThreadContext* in_thread_context,
                 std::unique_ptr<BaseEvent, FreeListAllocatorDeleter<BaseEvent>> in_base_event)
      : thread_context(in_thread_context),
        base_event(std::move(in_base_event)),
        timestamp(base_event->timestamp())
    {
    }

    /**
     * Constructor used for any events coming from the raw queue
     * @param in_thread_context thread context
     * @param in_timestamp timestamp for log message
     * @param in_serialization_metadata contains LogMacroMetadata and the deserialization info
     * @param in_logger_details logger details
     * @param in_fmt_store holds the arguments of the log message
     */
    TransitEvent(ThreadContext* in_thread_context, uint64_t in_timestamp,
                 detail::SerializationMetadata const* in_serialization_metadata,
                 detail::LoggerDetails const* in_logger_details,
                 fmt::dynamic_format_arg_store<fmt::format_context> in_fmt_store)
      : thread_context(in_thread_context),
        timestamp(in_timestamp),
        serialization_metadata(in_serialization_metadata),
        logger_details(in_logger_details),
        fmt_store(std::move(in_fmt_store))
    {
    }

    friend bool operator>(TransitEvent const& lhs, TransitEvent const& rhs)
    {
      return lhs.timestamp > rhs.timestamp;
    }

    ThreadContext* thread_context; /** We clean any invalidated thread_context after the priority queue is empty, so this can not be invalid */

    /**
     * TransitEvent is like a variant, it will contain a base_event is the event was pulled from the generic_queue
     * or it will contain metadata* and fmt dynamic store if the event was pulled from the fast_queue
     */
    std::unique_ptr<BaseEvent, FreeListAllocatorDeleter<BaseEvent>> base_event{nullptr};

    uint64_t timestamp; /** timestamp is populated for both events */
    detail::SerializationMetadata const* serialization_metadata{nullptr}; /** serialization metadata in case of fast_queue **/
    detail::LoggerDetails const* logger_details{nullptr};         /** The logger details **/
    fmt::dynamic_format_arg_store<fmt::format_context> fmt_store; /** fmt_store in case of fast_queue **/
  };

private:
  Config const& _config;
  ThreadContextCollection& _thread_context_collection;
  HandlerCollection const& _handler_collection;

  std::thread _backend_worker_thread; /** the backend thread that is writing the log to the handlers */
  uint32_t _backend_worker_thread_id{0}; /** cached backend worker thread id */

  std::unique_ptr<RdtscClock> _rdtsc_clock{nullptr}; /** rdtsc clock if enabled **/

  std::chrono::nanoseconds _backend_thread_sleep_duration; /** backend_thread_sleep_duration from config **/
  size_t _max_transit_events; /** limit of transit events before start flushing, value from config */

  bool _has_unflushed_messages{false}; /** There are messages that are buffered by the OS, but not yet flushed */
  std::atomic<bool> _is_running{false}; /** The spawned backend thread status */
  std::priority_queue<TransitEvent, std::vector<TransitEvent>, std::greater<>> _transit_events;

  BacktraceLogRecordStorage _backtrace_log_record_storage; /** Stores a vector of backtrace log records per logger name */

  FreeListAllocator _free_list_allocator; /** A free list allocator with initial capacity, we store the TransitEvents that we pop from each SPSC queue here */

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
  // We store the configuration here on our local variable since the config flag is not atomic
  // and we don't want it to change after we have started - This is just for safety and to
  // enforce the user to configure a variable before the thread has started
  _backend_thread_sleep_duration = _config.backend_thread_sleep_duration();
  _max_transit_events = _config.backend_thread_max_transit_events();

  std::thread worker(
    [this]()
    {
      QUILL_TRY
      {
        // On Start
        if (_config.backend_thread_cpu_affinity() != (std::numeric_limits<uint16_t>::max)())
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

      // Initialise memory for our free list allocator. We reserve the same size as a full
      // size of 1 caller thread queue
      _free_list_allocator.reserve(QUILL_QUEUE_CAPACITY);

      // Also configure our allocator to request bigger chunks from os
      _free_list_allocator.set_minimum_allocation(QUILL_QUEUE_CAPACITY);

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
}

/***/
void BackendWorker::_populate_priority_queue(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts,
                                             bool is_terminating)
{
  // copy everything to a priority queue
  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    // read the generic event queue
    _read_event_queue(thread_context, is_terminating);

#if !defined(QUILL_DISABLE_DUAL_QUEUE_MODE)
    // read the fast raw spsc queue
    _deserialize_raw_queue(thread_context, is_terminating);
#endif
  }
}

/***/
void BackendWorker::_read_event_queue(ThreadContext* thread_context, bool is_terminating)
{
  // Read the generic queue
  ThreadContext::EventSPSCQueueT& object_spsc_queue = thread_context->event_spsc_queue();

  while (true)
  {
    if (!is_terminating && (_transit_events.size() >= _max_transit_events))
    {
      // transit events queue is full
      break;
    }

    auto handle = object_spsc_queue.try_pop();

    if (!handle.is_valid())
    {
      // keep reading until the queue is empty or we reached the transit events limit
      break;
    }

    _transit_events.emplace(thread_context, handle.data()->clone(_free_list_allocator));
  }
}

#if !defined(QUILL_DISABLE_DUAL_QUEUE_MODE)
/***/
void BackendWorker::_deserialize_raw_queue(ThreadContext* thread_context, bool is_terminating)
{
  // Read the fast queue
  ThreadContext::RawSPSCQueueT& raw_spsc_queue = thread_context->raw_spsc_queue();

  while (true)
  {
    if (!is_terminating && (_transit_events.size() >= _max_transit_events))
    {
      // transit events queue is full
      break;
    }

    // Note: The producer will commit a write to this queue when one complete message is written.
    // This means that if we can read something from the queue it will be a full message
    // The producer will add items to the buffer :
    // |timestamp|log_data_node*|logger_details*|args...|

    // We want to read a minimum size of uint64_t (the size of the timestamp)
    auto const read_buffer_avail_bytes_pair = raw_spsc_queue.prepare_read();
    unsigned char const* read_buffer = read_buffer_avail_bytes_pair.first;
    size_t const bytes_available = read_buffer_avail_bytes_pair.second;

    if (bytes_available == 0)
    {
      // keep reading until the queue is empty or we reached the transit events limit
      break;
    }

    // read the next full message
    uint64_t timestamp;
    std::memcpy(&timestamp, read_buffer, sizeof(uint64_t));
    read_buffer += sizeof(uint64_t);

    uintptr_t serialization_metadata_ptr;
    std::memcpy(&serialization_metadata_ptr, read_buffer, sizeof(uintptr_t));
    auto const serialization_metadata =
      reinterpret_cast<detail::SerializationMetadata const*>(serialization_metadata_ptr);
    read_buffer += sizeof(uintptr_t);

    uintptr_t logger_details_ptr;
    std::memcpy(&logger_details_ptr, read_buffer, sizeof(uintptr_t));
    auto const logger_details = reinterpret_cast<detail::LoggerDetails const*>(logger_details_ptr);
    read_buffer += sizeof(uintptr_t);

    // Store all arguments
    fmt::dynamic_format_arg_store<fmt::format_context> fmt_store;
    size_t read_size = 0;
    for (char const type_descriptor : serialization_metadata->serialization_info)
    {
      read_size += deserialize_argument(read_buffer, fmt_store, static_cast<TypeDescriptor>(type_descriptor));
    }

    // Finish reading
    raw_spsc_queue.finish_read(sizeof(uint64_t) + sizeof(uintptr_t) + sizeof(uintptr_t) + read_size);

    // We have the timestamp and the data node ptr, we can construct a transit event out of them
    _transit_events.emplace(thread_context, timestamp, serialization_metadata, logger_details,
                            std::move(fmt_store));
  }
}
#endif

/***/
void BackendWorker::_process_transit_event()
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
    if (transit_event.base_event)
    {
      // This is a transit event coming from the generic_spsc_event_queue
      transit_event.base_event->backend_process(
        _backtrace_log_record_storage, transit_event.thread_context->thread_id(),
        transit_event.thread_context->thread_name(), obtain_active_handlers, get_real_ts);
    }
    else
    {
      // We are processing a transit event coming from the raw SPSC queue

      // TODO:: This is similar to _get_real_timestamp but not easy to make common and avoid the #if !defined(QUILL_CHRONO_CLOCK)
#if !defined(QUILL_CHRONO_CLOCK)
      std::chrono::nanoseconds const timestamp = _rdtsc_clock->time_since_epoch(transit_event.timestamp);
#else
      // Then the timestamp() will be already in epoch no need to convert it like above
      // The precision of system_clock::time-point is not portable across platforms.
      std::chrono::system_clock::duration const timestamp_duration{transit_event.timestamp};
      std::chrono::nanoseconds const timestamp = std::chrono::nanoseconds{timestamp_duration};
#endif

      // Forward the record to all of the logger handlers
      for (auto& handler : transit_event.logger_details->handlers())
      {
        handler->formatter().format(
          timestamp, transit_event.thread_context->thread_id(),
          transit_event.thread_context->thread_name(), transit_event.logger_details->name(),
          transit_event.serialization_metadata->log_macro_metadata, transit_event.fmt_store);

        // After calling format on the formatter we have to request the formatter record
        auto const& formatted_log_record_buffer = handler->formatter().formatted_log_record();

        // If all filters are okay we write this log record to the file
        if (handler->apply_filters(transit_event.thread_context->thread_id(), timestamp,
                                   transit_event.serialization_metadata->log_macro_metadata,
                                   formatted_log_record_buffer))
        {
          // log to the handler, also pass the log_record_timestamp this is only needed in some
          // cases like daily file rotation
          handler->write(formatted_log_record_buffer, timestamp,
                         transit_event.serialization_metadata->log_macro_metadata.level());
        }
      }

      // We also need to check the severity of the log message here against the backtrace
      // Check if we should also flush the backtrace messages:
      // After we forwarded the message we will check the severity of this message for this logger
      // If the severity of the message is higher than the backtrace flush severity we will also
      // flush the backtrace of the logger
      // NOTE: The backtrace messages were pushed in the Event queue, but they have been converted
      // to transit events anyway and then pushed into our priority queue.
      if (QUILL_UNLIKELY(transit_event.serialization_metadata->log_macro_metadata.level() >=
                         transit_event.logger_details->backtrace_flush_level()))
      {
        // process all records in backtrace for this logger_name and log them by calling backend_process_backtrace_log_record
        // note: we don't use obtain_active_handlers inside backend_process_backtrace_log_record,
        // we only use the handlers of the logger, but we just have to pass it because of the API
        _backtrace_log_record_storage.process(
          transit_event.logger_details->name(),
          [&obtain_active_handlers, &get_real_ts](std::string const& stored_thread_id, std::string const& stored_thread_name,
                                                  BaseEvent const* stored_backtrace_log_record)
          {
            // call backend process on each stored record
            stored_backtrace_log_record->backend_process_backtrace_log_record(
              stored_thread_id.data(), stored_thread_name.data(), obtain_active_handlers, get_real_ts);
          });
      }
    }

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

  _populate_priority_queue(cached_thread_contexts, false);

  if (QUILL_LIKELY(!_transit_events.empty()))
  {
    // the queue is not empty,
    if (_transit_events.size() >= _max_transit_events)
    {
      // process half transit events
      for (size_t i = 0; i < static_cast<size_t>(_max_transit_events / 2); ++i)
      {
        _process_transit_event();
      }
    }
    else
    {
      // process a single transit event, then populate priority queue again. This gives priority
      // to emptying the spsc queue from the hot threads as soon as possible
      _process_transit_event();
    }
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
  static_assert(
    std::is_same<BaseEvent::using_rdtsc, std::true_type>::value,
    "BaseEvent has a std::chrono timestamp, but the backend thread is using rdtsc timestamp");
  // pass to our clock the stored rdtsc from the caller thread
  return _rdtsc_clock->time_since_epoch(base_event->timestamp());
#else
  static_assert(
    std::is_same<BaseEvent::using_rdtsc, std::false_type>::value,
    "BaseEvent has an rdtsc timestamp, but the backend thread is using std::chrono timestamp");

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
    _populate_priority_queue(cached_thread_contexts, true);

    if (!_transit_events.empty())
    {
      // the queue is not empty,
      if (_transit_events.size() >= _max_transit_events)
      {
        // process half transit events
        for (size_t i = 0; i < static_cast<size_t>(_max_transit_events / 2); ++i)
        {
          _process_transit_event();
        }
      }
      else
      {
        // process a single transit event, then populate priority queue again. This gives priority
        // to emptying the spsc queue from the hot threads as soon as possible
        _process_transit_event();
      }
    }
    else
    {
      _check_dropped_messages(cached_thread_contexts);
      _force_flush();
      break;
    }
  }
}
} // namespace detail
} // namespace quill