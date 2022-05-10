/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/QuillError.h"               // for QUILL_CATCH, QUILL...
#include "quill/detail/Config.h"            // for Config
#include "quill/detail/HandlerCollection.h" // for HandlerCollection
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/Serialize.h"
#include "quill/detail/ThreadContext.h"            // for ThreadContext, Thr...
#include "quill/detail/ThreadContextCollection.h"  // for ThreadContextColle...
#include "quill/detail/backend/BacktraceStorage.h" // for BacktraceStorage
#include "quill/detail/misc/Attributes.h"          // for QUILL_ATTRIBUTE_HOT
#include "quill/detail/misc/Common.h"              // for QUILL_RDTSC_RESYNC...
#include "quill/detail/misc/Common.h"              // for QUILL_LIKELY
#include "quill/detail/misc/Os.h"                  // for set_cpu_affinity, get_thread_id
#include "quill/detail/misc/RdtscClock.h"          // for RdtscClock
#include "quill/detail/misc/Utilities.h"
#include "quill/detail/spsc_queue/UnboundedQueue.h"
#include "quill/handlers/Handler.h" // for Handler
#include <atomic>                   // for atomic, memory_ord...
#include <cassert>                  // for assert
#include <chrono>                   // for nanoseconds, milli...
#include <cstdint>                  // for uint16_t
#include <exception>                // for exception
#include <functional>               // for greater, function
#include <limits>                   // for numeric_limits
#include <memory>                   // for unique_ptr, make_u...
#include <queue>                    // for priority_queue
#include <string>                   // for allocator, string
#include <thread>                   // for sleep_for, thread
#include <utility>                  // for move
#include <vector>                   // for vector

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

  /**
   * Deserialize an log message from the raw SPSC queue and emplace them to priority queue
   */
  QUILL_ATTRIBUTE_HOT inline void _read_queue_and_decode(ThreadContext* thread_context, bool is_terminating);

  /**
   * Checks for events in all queues and processes the one with the minimum timestamp
   */
  QUILL_ATTRIBUTE_HOT inline void _process_transit_event();

  QUILL_ATTRIBUTE_HOT inline void _write_transit_event(TransitEvent const& transit_event);

  /**
   * Force flush all active Handlers
   */
  QUILL_ATTRIBUTE_HOT inline void _force_flush();

  /**
   * Check for dropped messages - only when bounded queue is used
   * @param cached_thread_contexts loaded thread contexts
   */
  QUILL_ATTRIBUTE_HOT static void _check_dropped_messages(
    ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts) noexcept;

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

  fmt::memory_buffer _membuf;                                    /** Format buffer */
  std::vector<fmt::basic_format_arg<fmt::format_context>> _args; /** Format args */

  BacktraceStorage _backtrace_log_record_storage; /** Stores a vector of backtrace log records per logger name */

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

      if constexpr (std::is_same<detail::Header::using_rdtsc, std::true_type>::value)
      {
        // Use rdtsc clock based on config. The clock requires a few seconds to init as it is
        // taking samples first
        _rdtsc_clock = std::make_unique<RdtscClock>(std::chrono::milliseconds{QUILL_RDTSC_RESYNC_INTERVAL});
      }

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
}

/***/
void BackendWorker::_populate_priority_queue(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts,
                                             bool is_terminating)
{
  // copy everything to a priority queue
  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    _read_queue_and_decode(thread_context, is_terminating);
  }
}

/***/
void BackendWorker::_read_queue_and_decode(ThreadContext* thread_context, bool is_terminating)
{
  // Read the fast queue
  ThreadContext::SPSCQueueT& spsc_queue = thread_context->spsc_queue();

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
    // |timestamp|metadata*|logger_details*|args...|

    auto read = spsc_queue.prepare_read();
    std::byte* read_buffer = read.first;
    size_t bytes_available = read.second;
    std::byte* const read_begin = read_buffer;

    if (bytes_available == 0)
    {
      // keep reading until the queue is empty or we reached the transit events limit
      break;
    }

    // read the next full message
    read_buffer = detail::align_pointer<alignof(Header), std::byte>(read_buffer);
    detail::Header header = *(reinterpret_cast<detail::Header*>(read_buffer));
    read_buffer += sizeof(detail::Header);

    // it is a flush event, we store the pointer of the passed in variable to the transit event
    std::atomic<bool>* flush_flag{nullptr};

    // we need to check and do not try to format the flush events as that wouldn't be valid
    if (header.metadata->macro_metadata.event() != MacroMetadata::Event::Flush)
    {
      read_buffer = header.metadata->format_to_fn(header.metadata->macro_metadata.message_format(),
                                                  read_buffer, _membuf, _args);
    }
    else
    {
      // if this is a flush event then we do not need to format anything for the
      // transit_event, but we need to set the transit event's flush_flag pointer instead
      uintptr_t flush_flag_tmp;
      std::memcpy(&flush_flag_tmp, read_buffer, sizeof(uintptr_t));
      flush_flag = reinterpret_cast<std::atomic<bool>*>(flush_flag_tmp);
      read_buffer += sizeof(uintptr_t);
    }

    // Finish reading
    spsc_queue.finish_read(static_cast<uint64_t>(read_buffer - read_begin));

    // We have the timestamp and the data node ptr, we can construct a transit event out of them
    _transit_events.emplace(thread_context, header, std::move(_membuf), flush_flag);
  }
}

/***/
void BackendWorker::_process_transit_event()
{
  TransitEvent const& transit_event = _transit_events.top();

  // If backend_process(...) throws we want to skip this event and move to the next so we catch the
  // error here instead of catching it in the parent try/catch block of main_loop
  QUILL_TRY
  {
    if (transit_event.header.metadata->macro_metadata.event() == MacroMetadata::Event::Log)
    {
      if (transit_event.header.metadata->macro_metadata.level() != LogLevel::Backtrace)
      {
        _write_transit_event(transit_event);

        // We also need to check the severity of the log message here against the backtrace
        // Check if we should also flush the backtrace messages:
        // After we forwarded the message we will check the severity of this message for this logger
        // If the severity of the message is higher than the backtrace flush severity we will also
        // flush the backtrace of the logger
        if (QUILL_UNLIKELY(transit_event.header.metadata->macro_metadata.level() >=
                           transit_event.header.logger_details->backtrace_flush_level()))
        {
          _backtrace_log_record_storage.process(transit_event.header.logger_details->name(),
                                                [this](TransitEvent const& transit_event)
                                                { _write_transit_event(transit_event); });
        }
      }
      else
      {
        // this is a backtrace log and we will store it
        _backtrace_log_record_storage.store(transit_event);
      }
    }
    else if (transit_event.header.metadata->macro_metadata.event() == MacroMetadata::Event::InitBacktrace)
    {
      // we can just convert the capacity back to int here and use it
      _backtrace_log_record_storage.set_capacity(
        transit_event.header.logger_details->name(),
        static_cast<uint32_t>(std::stoul(
          std::string{transit_event.formatted_msg.begin(), transit_event.formatted_msg.end()})));
    }
    else if (transit_event.header.metadata->macro_metadata.event() == MacroMetadata::Event::FlushBacktrace)
    {
      // process all records in backtrace for this logger_name and log them by calling backend_process_backtrace_log_record
      _backtrace_log_record_storage.process(transit_event.header.logger_details->name(),
                                            [this](TransitEvent const& transit_event)
                                            { _write_transit_event(transit_event); });
    }
    else if (transit_event.header.metadata->macro_metadata.event() == MacroMetadata::Event::Flush)
    {
      _force_flush();

      // this is a flush event so we need to notify the caller to continue now
      transit_event.flush_flag->store(true);
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

/***/
void BackendWorker::_write_transit_event(TransitEvent const& transit_event)
{
  std::chrono::nanoseconds timestamp;
  if constexpr (std::is_same<detail::Header::using_rdtsc, std::true_type>::value)
  {
    timestamp = _rdtsc_clock->time_since_epoch(transit_event.header.timestamp);
  }
  else
  {
    // Then the timestamp() will be already in epoch no need to convert it like above
    // The precision of system_clock::time-point is not portable across platforms.
    std::chrono::system_clock::duration const timestamp_duration{transit_event.header.timestamp};
    timestamp = std::chrono::nanoseconds{timestamp_duration};
  }

  // Forward the record to all of the logger handlers
  for (auto& handler : transit_event.header.logger_details->handlers())
  {
    handler->formatter().format(timestamp, transit_event.thread_id.data(), transit_event.thread_name.data(),
                                transit_event.header.logger_details->name(),
                                transit_event.header.metadata->macro_metadata, transit_event.formatted_msg);

    // After calling format on the formatter we have to request the formatter record
    auto const& formatted_log_record_buffer = handler->formatter().formatted_log_record();

    // If all filters are okay we write this log record to the file
    if (handler->apply_filters(transit_event.thread_id.data(), timestamp,
                               transit_event.header.metadata->macro_metadata, formatted_log_record_buffer))
    {
      // log to the handler, also pass the log_record_timestamp this is only needed in some
      // cases like daily file rotation
      handler->write(formatted_log_record_buffer, timestamp,
                     transit_event.header.metadata->macro_metadata.level());
    }
  }
}

/***/
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