/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#if defined(_WIN32)
  #include "quill/backend/Utf8Conv.h"
#endif

#include "quill/backend/BackendMdcState.h"
#include "quill/backend/BackendOptions.h"
#include "quill/backend/BackendUtilities.h"
#include "quill/backend/BackendWorkerLock.h"
#include "quill/backend/BacktraceStorage.h"
#include "quill/backend/PatternFormatter.h"
#include "quill/backend/RdtscClock.h"
#include "quill/backend/ThreadUtilities.h"
#include "quill/backend/TransitEvent.h"
#include "quill/backend/TransitEventBuffer.h"

#include "quill/core/Attributes.h"
#include "quill/core/BoundedSPSCQueue.h"
#include "quill/core/ChronoTimeUtils.h"
#include "quill/core/Codec.h"
#include "quill/core/Common.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/LogLevel.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/LoggerManager.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/MathUtilities.h"
#include "quill/core/Metric.h"
#include "quill/core/QuillError.h"
#include "quill/core/SinkManager.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/core/ThreadPrimitives.h"
#include "quill/core/TimeUtilities.h"
#include "quill/core/UnboundedSPSCQueue.h"
#include "quill/sinks/Sink.h"

#include "quill/bundled/fmt/base.h"

#include <algorithm>
#include <atomic>
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
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT
class ManualBackendWorker; // Forward declaration
QUILL_END_EXPORT

namespace detail
{

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4324)
#endif

class BackendWorker
{
public:
  /**
   * Constructor
   */
  BackendWorker() = default;

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

    RdtscClock const* rdtsc_clock = _rdtsc_clock.exchange(nullptr);
    delete rdtsc_clock;
  }

  /***/
  QUILL_NODISCARD bool is_running() const noexcept
  {
    return _is_worker_running.load(std::memory_order_acquire);
  }

  /**
   * Access the rdtsc class from any thread to convert an rdtsc value to wall time
   */
  QUILL_NODISCARD uint64_t time_since_epoch(uint64_t rdtsc_value) const
  {
    if (QUILL_UNLIKELY(_options.sleep_duration > _options.rdtsc_resync_interval))
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
    return _worker_thread_id.load();
  }

  /***/
  QUILL_ATTRIBUTE_COLD static void validate_options(BackendOptions const& options)
  {
    (void)BackendMdcState{options.mdc_format_pattern};
    _validate_transit_event_limits(options);
  }

  /**
   * Starts the backend worker thread
   * @throws std::runtime_error, std::system_error on failures
   */
  QUILL_ATTRIBUTE_COLD void run(BackendOptions const& options)
  {
    _ensure_linker_retains_symbols();

    _has_worker_thread_exited.store(false);

    _process_id = std::to_string(get_process_id());

    validate_options(options);

    if (options.check_backend_singleton_instance)
    {
      _backend_worker_lock = std::make_unique<BackendWorkerLock>(_process_id);
    }

    std::thread worker(
      [this, options]()
      {
        QUILL_TRY { _init(options); }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e)
        {
          _notify_error(options.error_notifier, e.what());
          std::terminate();
        }
        QUILL_CATCH_ALL()
        {
          _notify_error(options.error_notifier,
                        std::string{"Caught unhandled exception during backend initialization."});
          std::terminate();
        }
#endif

        QUILL_TRY
        {
          if (!_options.cpu_affinity.empty())
          {
            // Set cpu affinity if requested
            set_cpu_affinity(_options.cpu_affinity);
          }
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        // The backend continues running without the affinity, prefix as a warning
        QUILL_CATCH(std::exception const& e)
        {
          _notify_error(_options.error_notifier, _get_local_time_str() + " Quill WARNING: " + e.what());
        }
        QUILL_CATCH_ALL()
        {
          _notify_error(_options.error_notifier,
                        _get_local_time_str() + " Quill WARNING: Caught unhandled exception.");
        }
#endif

        QUILL_TRY
        {
          // Set the thread name to the desired name
          set_thread_name(_options.thread_name.data());
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        // The backend continues running without the thread name, prefix as a warning
        QUILL_CATCH(std::exception const& e)
        {
          _notify_error(_options.error_notifier, _get_local_time_str() + " Quill WARNING: " + e.what());
        }
        QUILL_CATCH_ALL()
        {
          _notify_error(_options.error_notifier,
                        _get_local_time_str() + " Quill WARNING: Caught unhandled exception.");
        }
#endif

        // All okay, set the backend worker thread running flag
        _is_worker_running.store(true);

        // Running
        while (QUILL_LIKELY(_is_worker_running.load(std::memory_order_relaxed)))
        {
          // main loop
          QUILL_TRY { _poll(); }
#if !defined(QUILL_NO_EXCEPTIONS)
          QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
          QUILL_CATCH_ALL()
          {
            _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
          }
#endif
        }

        // Synchronise with BackendWorker::stop() before tearing down backend-owned state. The loop
        // above intentionally uses a relaxed load on the hot path; this acquire load is cold and
        // pairs with stop()'s exchange(false) so Quill API calls sequenced-before stop()
        // happen-before _exit() deletes resources such as RdtscClock.
        QUILL_MAYBE_UNUSED bool const stopped = _is_worker_running.load(std::memory_order_acquire);
        QUILL_ASSERT(!stopped,
                     "Backend worker should only exit after stop() clears the running flag");

        // exit
        QUILL_TRY { _exit(); }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
        QUILL_CATCH_ALL()
        {
          _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
        }
#endif

        _has_worker_thread_exited.store(true);
      });

    // Move the worker ownership to our class
    _worker_thread.swap(worker);

    while (!_is_worker_running.load(std::memory_order_seq_cst))
    {
      // wait for the thread to start
      detail::sleep_for_ns(100'000ull); // 100 us
    }
  }

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop() noexcept
  {
    // Stop the backend worker
    if (!_is_worker_running.exchange(false))
    {
      // already stopped
      return;
    }

    // signal wake up the backend worker thread
    notify();

    // Wait the backend thread to join, if backend thread was never started it won't be joinable
    if (_worker_thread.joinable())
    {
      _worker_thread.join();
    }

    if (!_has_worker_thread_exited.load())
    {
      // The backend thread was terminated externally without completing its exit sequence.
      // This can happen on Windows when Quill is used inside a DLL
      // Drain any remaining log messages on this thread as a best-effort fallback.
      QUILL_TRY { _exit(); }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
      QUILL_CATCH_ALL()
      {
        _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
      }
#endif
    }

    _worker_thread_id.store(0);
    _backend_worker_lock.reset(nullptr);

    // Release scratch state owned by the worker so a subsequent start() begins from a clean
    // slate and we don't hold on to memory grown during the previous run. The frontend queues,
    // sinks, and loggers themselves are owned by the singleton managers and intentionally
    // survive restarts; only the worker's own scratch state is freed here. The worker thread
    // has already joined above so these members are not concurrently accessed. Reassigning
    // from a fresh empty instance both clears the contents and releases any reserved capacity.
    _named_args_templates = {};
    _logger_removal_flags = {};
    _removed_loggers = {};
    _active_thread_contexts_cache = {};
    _active_sinks_cache = {};
    _named_args_format_template = {};
    _format_args_store = DynamicFormatArgStore{};
  }

  /**
   * Wakes up the backend worker thread.
   * Thread safe to be called from any thread
   */
  void notify()
  {
#ifdef __MINGW32__
    // MinGW can deadlock if the mutex is released before cv.notify_one(),
    // so keep notify_one() inside the lock for MinGW
    std::lock_guard<std::mutex> lock{_wake_up_mutex};
    _wake_up_flag = true;
    _wake_up_cv.notify_one();
#else
    // Set the flag to indicate that the data is ready
    {
      std::lock_guard<std::mutex> lock{_wake_up_mutex};
      _wake_up_flag = true;
    }

    // Signal the condition variable to wake up the worker thread
    _wake_up_cv.notify_one();
#endif
  }

private:
  /***/
  QUILL_ATTRIBUTE_HOT void _invoke_poll_hook(std::function<void()> const& hook) const
  {
    QUILL_TRY { hook(); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
    QUILL_CATCH_ALL()
    {
      _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
    }
#endif
  }

  /***/
  QUILL_ATTRIBUTE_HOT void _invoke_poll_end_once(bool& poll_end_called) const
  {
    if (poll_end_called)
    {
      return;
    }

    poll_end_called = true;
    _invoke_poll_hook(_options.backend_worker_on_poll_end);
  }

  /**
   * Formats the current local time as "HH:MM:SS", matching the prefix used by the
   * "Quill INFO:" queue notifications
   */
  QUILL_NODISCARD static std::string _get_local_time_str()
  {
    char ts[24];
    time_t t = time(nullptr);
    tm p;
    localtime_rs(std::addressof(t), std::addressof(p));
    strftime(ts, 24, "%X", std::addressof(p));
    return std::string{ts};
  }

  template <typename TMessage>
  static void _notify_error(std::function<void(std::string const&)> const& error_notifier, TMessage&& message)
  {
    if (static_cast<bool>(error_notifier))
    {
      QUILL_TRY { error_notifier(static_cast<TMessage&&>(message)); }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH_ALL()
      {
        // Swallow exceptions from the user-provided error_notifier to prevent
        // infinite loops when both formatting and the notifier throw.
      }
#endif
    }
  }

  /**
   * Calls some functions that we forward declare on the backend and tries to ensure the linker
   * includes the necessary symbols
   */
  QUILL_ATTRIBUTE_COLD static void _ensure_linker_retains_symbols()
  {
    // Calls to ensure it is retained by the linker.
    QUILL_MAYBE_UNUSED static auto thread_name = get_thread_name();
    (void)thread_name;

#if defined(_WIN32)
    std::wstring const dummy = L"dummy";
    QUILL_MAYBE_UNUSED static auto encode1 = utf8_encode(dummy);
    (void)encode1;

    QUILL_MAYBE_UNUSED static auto encode2 =
      utf8_encode(reinterpret_cast<std::byte const*>(dummy.data()), dummy.size());
    (void)encode2;
#endif
  }

  /**
   * Backend worker thread main function
   */
  QUILL_ATTRIBUTE_HOT void _poll()
  {
    // poll hook
    bool poll_end_called = false;
    if (QUILL_UNLIKELY(static_cast<bool>(_options.backend_worker_on_poll_begin)))
    {
      _invoke_poll_hook(_options.backend_worker_on_poll_begin);
    }

    // load all contexts locally
    _update_active_thread_contexts_cache();

    // Read all frontend queues and cache the log statements and the metadata as TransitEvents
    size_t const cached_transit_events_count = _populate_transit_events_from_frontend_queues();

    if (cached_transit_events_count != 0)
    {
      // there are cached events to process
      if (cached_transit_events_count < _options.transit_events_soft_limit)
      {
        // process a single transit event, then give priority to reading the frontend queues again
        _process_lowest_timestamp_transit_event();
      }
      else
      {
        // we want to process a batch of events.
        while (!has_pending_events_for_caching_when_transit_event_buffer_empty() &&
               _process_lowest_timestamp_transit_event())
        {
          // We need to be cautious because there are log messages in the lock-free queues
          // that have not yet been cached in the transit event buffer. Logging only the cached
          // messages can result in out-of-order log entries, as messages with lower timestamps
          // in the queue might be missed.
        }
      }
    }
    else
    {
      // No cached transit events to process, minimal thread workload.

      // force flush all remaining messages
      _flush_and_run_active_sinks(true, _options.sink_min_flush_interval);

      // check for any dropped events / blocked threads
      _check_failure_counter(_options.error_notifier);

      // This is useful when BackendTscClock is used to keep it up to date
      _resync_rdtsc_clock();

      // Also check if all queues are empty
      bool const queues_and_events_empty = _check_frontend_queues_and_cached_transit_events_empty();
      if (queues_and_events_empty)
      {
        _cleanup_invalidated_thread_contexts();
        _cleanup_invalidated_loggers();
        _try_shrink_empty_transit_event_buffers();

        // poll hook
        if (QUILL_UNLIKELY(static_cast<bool>(_options.backend_worker_on_poll_end)))
        {
          _invoke_poll_end_once(poll_end_called);
        }

        // There is nothing left to do, and we can let this thread sleep for a while
        // buffer events are 0 here and also all the producer queues are empty
        if (_options.sleep_duration.count() != 0)
        {
          std::unique_lock<std::mutex> lock{_wake_up_mutex};

          // Wait for a timeout or a notification to wake up
          _wake_up_cv.wait_for(lock, _options.sleep_duration, [this] { return _wake_up_flag; });

          // set the flag back to false since we woke up here
          _wake_up_flag = false;

          // After waking up resync rdtsc clock again and resume
          _resync_rdtsc_clock();
        }
        else if (_options.enable_yield_when_idle)
        {
          detail::yield_thread();
        }
      }
    }

    // poll hook
    if (QUILL_UNLIKELY(static_cast<bool>(_options.backend_worker_on_poll_end)))
    {
      _invoke_poll_end_once(poll_end_called);
    }
  }

  /***/
  QUILL_ATTRIBUTE_COLD static void _validate_transit_event_limits(BackendOptions const& options)
  {
    size_t const soft_limit = (options.transit_events_soft_limit == 0) ? 1 : options.transit_events_soft_limit;
    size_t const hard_limit = (options.transit_events_hard_limit == 0) ? 1 : options.transit_events_hard_limit;

    if (soft_limit > hard_limit)
    {
      QUILL_THROW(QuillError{fmtquill::format(
        "transit_events_soft_limit ({}) cannot be greater than transit_events_hard_limit "
        "({}). Please ensure that the soft limit is less than or equal to the hard limit.",
        soft_limit, hard_limit)});
    }

    if (!is_power_of_two(hard_limit))
    {
      QUILL_THROW(QuillError{
        fmtquill::format("transit_events_hard_limit ({}) must be a power of two", hard_limit)});
    }

    size_t const requested_initial_capacity = static_cast<size_t>(options.transit_event_buffer_initial_capacity);
    size_t const rounded_initial_capacity = next_power_of_two(requested_initial_capacity);
    if ((requested_initial_capacity > hard_limit) || (rounded_initial_capacity > hard_limit))
    {
      QUILL_THROW(QuillError{fmtquill::format(
        "transit_event_buffer_initial_capacity ({}, rounded to {}) cannot be greater than "
        "transit_events_hard_limit ({})",
        options.transit_event_buffer_initial_capacity, rounded_initial_capacity, hard_limit)});
    }
  }

  /**
   * Logging thread init function
   */
  QUILL_ATTRIBUTE_COLD void _init(BackendOptions const& options)
  {
    _options = options;

    // ManualBackendWorker::init() calls _init() directly, so validate here as well.
    validate_options(_options);

    (void)get_thread_name();

    // Zero limits make no sense as we can't process anything; normalize them to 1
    if (_options.transit_events_hard_limit == 0)
    {
      _options.transit_events_hard_limit = 1;
    }

    if (_options.transit_events_soft_limit == 0)
    {
      _options.transit_events_soft_limit = 1;
    }

    _last_output_timestamp = 0;

    // Backend::stop() releases the worker's cache, but thread-local contexts can
    // outlive the backend thread and be reused after a later Backend::start().
    // Refresh unconditionally so existing frontend queues are visible again.
    _update_active_thread_contexts_cache(true);

    // Cache this thread's id only after initialization has succeeded. ManualBackendWorker::init()
    // calls this function on the caller thread and can propagate validation errors.
    uint32_t const worker_thread_id = get_thread_id();
    _worker_thread_id.store(worker_thread_id);
    LoggerBase::set_current_thread_is_backend_thread(true);
  }

  /***/
  void _clear_backend_thread_flag() noexcept
  {
    LoggerBase::set_current_thread_is_backend_thread(false);
  }

  /**
   * Logging thread exit function that flushes everything after stop() is called
   */
  QUILL_ATTRIBUTE_COLD void _exit()
  {
    while (true)
    {
      bool const queues_and_events_empty = (!_options.wait_for_queues_to_empty_before_exit) ||
        _check_frontend_queues_and_cached_transit_events_empty();

      if (queues_and_events_empty)
      {
        // we are done, all queues are now empty
        _check_failure_counter(_options.error_notifier);
        _flush_and_run_active_sinks(false, std::chrono::milliseconds{0});
        break;
      }

      uint64_t const cached_transit_events_count = _populate_transit_events_from_frontend_queues();
      if (cached_transit_events_count > 0)
      {
        while (!has_pending_events_for_caching_when_transit_event_buffer_empty() &&
               _process_lowest_timestamp_transit_event())
        {
          // We need to be cautious because there are log messages in the lock-free queues
          // that have not yet been cached in the transit event buffer. Logging only the cached
          // messages can result in out-of-order log entries, as messages with lower timestamps
          // in the queue might be missed.
        }
      }
    }

    _cleanup_invalidated_thread_contexts();
    _cleanup_invalidated_loggers();

    // Tear down the RdtscClock so a subsequent Backend::start() can recalibrate
    // using the current options (e.g. a different rdtsc_resync_interval). The
    // documented contract of Backend::stop() is that no other Quill API may run
    // concurrently with it, including BackendTscClock::now()/to_time_point on
    // user threads, so this is safe.
    RdtscClock const* rdtsc_clock = _rdtsc_clock.exchange(nullptr, std::memory_order_acq_rel);
    delete rdtsc_clock;

    _clear_backend_thread_flag();
  }

  /**
   * Populates the local transit event buffer
   */
  QUILL_ATTRIBUTE_HOT size_t _populate_transit_events_from_frontend_queues()
  {
    uint64_t const ts_now = _options.log_timestamp_ordering_grace_period.count()
      ? (detail::get_system_time_ns() -
         static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(_options.log_timestamp_ordering_grace_period)
                                 .count()))
      : (std::numeric_limits<uint64_t>::max)();

    size_t total_cached_transit_events_count{0};

    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      QUILL_ASSERT(thread_context->has_unbounded_queue_type() || thread_context->has_bounded_queue_type(),
                   "Invalid queue type in BackendWorker::_read_and_decode_frontend_queues()");

      if (thread_context->has_unbounded_queue_type())
      {
        total_cached_transit_events_count += _read_and_decode_frontend_queue(
          thread_context->get_spsc_queue_union().unbounded_spsc_queue, thread_context, ts_now);
      }
      else if (thread_context->has_bounded_queue_type())
      {
        total_cached_transit_events_count += _read_and_decode_frontend_queue(
          thread_context->get_spsc_queue_union().bounded_spsc_queue, thread_context, ts_now);
      }
    }

    return total_cached_transit_events_count;
  }

  /**
   * Deserialize messages from the raw SPSC queue
   * @param frontend_queue queue
   * @param thread_context thread context
   * @param ts_now timestamp now
   * @return size of the transit_event_buffer
   */
  template <typename TFrontendQueue>
  QUILL_ATTRIBUTE_HOT size_t _read_and_decode_frontend_queue(TFrontendQueue& frontend_queue,
                                                             ThreadContext* thread_context, uint64_t ts_now)
  {
    // Note: The producer commits only complete messages to the queue.
    // Therefore, if even a single byte is present in the queue, it signifies a full message.
    size_t const queue_capacity = frontend_queue.capacity();
    size_t total_bytes_read{0};

    // Reads a maximum of one full frontend queue or up to the transit events' hard limit to
    // prevent getting stuck on the same producer. The buffer size is checked before each read so
    // that a buffer already at the hard limit does not accept another event and expand past it
    while ((total_bytes_read < queue_capacity) &&
           (thread_context->_transit_event_buffer->size() < _options.transit_events_hard_limit))
    {
      std::byte* read_pos;

      if constexpr (std::is_same_v<TFrontendQueue, UnboundedSPSCQueue>)
      {
        read_pos = _read_unbounded_frontend_queue(frontend_queue, thread_context);
      }
      else
      {
        read_pos = frontend_queue.prepare_read();
      }

      if (!read_pos)
      {
        // Exit loop nothing to read
        break;
      }

      std::byte const* const read_begin = read_pos;

      if (!_populate_transit_event_from_frontend_queue(read_pos, thread_context, ts_now))
      {
        // If _get_transit_event_from_queue returns false, stop reading
        break;
      }

      // Finish reading
      QUILL_ASSERT(
        read_pos >= read_begin,
        "read_pos must be >= read_begin in BackendWorker::_read_and_decode_frontend_queue()");

      auto const bytes_read = static_cast<size_t>(read_pos - read_begin);
      frontend_queue.finish_read(bytes_read);
      total_bytes_read += bytes_read;
    }

    if (total_bytes_read != 0)
    {
      // If we read something from the queue, we commit all the reads together at the end.
      // This strategy enhances cache coherence performance by updating the shared atomic flag
      // only once.
      frontend_queue.commit_read();
    }

    return thread_context->_transit_event_buffer->size();
  }

  /***/
  QUILL_ATTRIBUTE_HOT bool _populate_transit_event_from_frontend_queue(std::byte*& read_pos,
                                                                       ThreadContext* thread_context,
                                                                       uint64_t ts_now)
  {
    QUILL_ASSERT(thread_context->_transit_event_buffer,
                 "transit_event_buffer is nullptr in "
                 "BackendWorker::_populate_transit_event_from_frontend_queue()");

    // Allocate a new TransitEvent or use an existing one to store the message from the queue
    TransitEvent* transit_event{nullptr};

    QUILL_TRY { transit_event = thread_context->_transit_event_buffer->back(); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      // Growing the transit event buffer failed (allocation failure). Nothing has been consumed
      // from the queue yet, so leave the message where it is and stop reading this queue. The
      // poll loop keeps processing already-cached events, which frees buffer slots so a later
      // attempt succeeds at the current capacity without allocating.
      // Pass a literal here: formatting a message could allocate and throw again, and an
      // exception escaping this handler would skip event processing and prevent recovery.
      _notify_error(_options.error_notifier,
                    "Quill ERROR: Failed to expand the transit event buffer (allocation failure). "
                    "The log message remains in the thread queue and will be retried");
      return false;
    }
#endif

    QUILL_ASSERT(
      transit_event,
      "transit_event is nullptr in BackendWorker::_populate_transit_event_from_frontend_queue()");

    QUILL_ASSERT(
      transit_event->formatted_msg,
      "formatted_msg is nullptr in BackendWorker::_populate_transit_event_from_frontend_queue()");

    static_assert(sizeof(FormatArgsDecoder) == sizeof(uintptr_t),
                  "FormatArgsDecoder must fit in uintptr_t for packed header decoding");
    static_assert(sizeof(uintptr_t) <= sizeof(uint64_t),
                  "Packed header decoding requires pointers to fit in 64 bits");

    uint64_t header_words[4];
    std::memcpy(header_words, read_pos, sizeof(header_words));
    read_pos += sizeof(header_words);

    transit_event->timestamp = header_words[0];
    transit_event->macro_metadata =
      reinterpret_cast<MacroMetadata const*>(static_cast<uintptr_t>(header_words[1]));
    transit_event->logger_base = reinterpret_cast<LoggerBase*>(static_cast<uintptr_t>(header_words[2]));

    QUILL_ASSERT(transit_event->logger_base,
                 "transit_event->logger_base is nullptr after memcpy from queue");

    QUILL_ASSERT(transit_event->logger_base->_clock_source == ClockSourceType::Tsc ||
                   transit_event->logger_base->_clock_source == ClockSourceType::System ||
                   transit_event->logger_base->_clock_source == ClockSourceType::User,
                 "transit_event->logger_base->_clock_source has invalid enum value - possible "
                 "memory corruption");

    if (transit_event->logger_base->_clock_source == ClockSourceType::Tsc)
    {
      // If using the rdtsc clock, convert the value to nanoseconds since epoch.
      // This conversion ensures that every transit inserted in the buffer below has a timestamp in
      // nanoseconds since epoch, allowing compatibility with Logger objects using different clocks.
      if (QUILL_UNLIKELY(!_rdtsc_clock.load(std::memory_order_relaxed)))
      {
        // Lazy initialization of rdtsc clock on the backend thread only if the user decides to use
        // it. The clock requires a few seconds to init as it is taking samples first.
        _rdtsc_clock.store(new RdtscClock{_options.rdtsc_resync_interval}, std::memory_order_release);
        _last_rdtsc_resync_time = std::chrono::steady_clock::now();
      }

      // Convert the rdtsc value to nanoseconds since epoch.
      transit_event->timestamp =
        _rdtsc_clock.load(std::memory_order_relaxed)->time_since_epoch(transit_event->timestamp);
    }

    // Check if strict log timestamp order is enabled and the clock source is not User
    if ((transit_event->logger_base->_clock_source != ClockSourceType::User) &&
        (ts_now != (std::numeric_limits<uint64_t>::max)()))
    {
      // We only check against `ts_now` for real timestamps, not custom timestamps by the user, and
      // when the grace period is enabled

#if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)
      // Check the timestamps we are comparing have the same digits
      auto count_digits = [](uint64_t number)
      {
        uint32_t digits = 0;
        do
        {
          digits++;
          number /= 10;
        } while (number != 0);
        return digits;
      };

      QUILL_ASSERT(count_digits(transit_event->timestamp) == count_digits(ts_now),
                   "Timestamp digits mismatch in "
                   "BackendWorker::_populate_transit_event_from_frontend_queue(), log timestamp "
                   "has different magnitude than current time");
#endif

      // Ensure the message timestamp is not greater than ts_now.
      if (QUILL_UNLIKELY(transit_event->timestamp > ts_now))
      {
        // If the message timestamp is ahead of the grace-period cutoff, temporarily halt
        // processing this queue. This keeps newer events in the queue until other frontend
        // threads have had the configured grace window to publish older timestamped events.
        // We return at this point without adding the current event to the buffer.
        return false;
      }
    }

    bool const is_mdc_event = (transit_event->macro_metadata->event() == MacroMetadata::Event::MdcSet) ||
      (transit_event->macro_metadata->event() == MacroMetadata::Event::MdcErase) ||
      (transit_event->macro_metadata->event() == MacroMetadata::Event::MdcClear);

    if (transit_event->macro_metadata->event() != MacroMetadata::Event::Metric)
    {
      uintptr_t const decoder_bits = static_cast<uintptr_t>(header_words[3]);
      FormatArgsDecoder format_args_decoder;
      std::memcpy(&format_args_decoder, &decoder_bits, sizeof(format_args_decoder));

      if (is_mdc_event)
      {
        _apply_mdc_event(*thread_context, transit_event->macro_metadata->event(), format_args_decoder, read_pos);
      }
      else
      {
        bool const runtime_metadata_event =
          (transit_event->macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy) ||
          (transit_event->macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy) ||
          (transit_event->macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy);

        if (runtime_metadata_event)
        {
          _apply_runtime_metadata(read_pos, transit_event);
        }

        // we need to check and do not try to format the flush events as that wouldn't be valid
        if ((transit_event->macro_metadata->event() != MacroMetadata::Event::Flush) &&
            (transit_event->macro_metadata->event() != MacroMetadata::Event::LoggerRemovalRequest))
        {
          format_args_decoder(read_pos, _format_args_store);

          if (!transit_event->macro_metadata->has_named_args())
          {
            _populate_formatted_log_message(transit_event, transit_event->macro_metadata->message_format());
          }
          else if (runtime_metadata_event)
          {
            // Runtime metadata format strings are user generated and can be unique per call;
            // caching them would grow _named_args_templates without bound for the lifetime of
            // the backend, so process them without caching
            auto const [message_format, arg_names] =
              _process_named_args_format_message(transit_event->macro_metadata->message_format());

            _populate_formatted_log_message(transit_event, message_format.data());
            _populate_formatted_named_args(transit_event, arg_names);
          }
          else
          {
            // using the message_format as key for lookups
            _named_args_format_template.assign(transit_event->macro_metadata->message_format());

            if (auto const search = _named_args_templates.find(_named_args_format_template);
                search != std::cend(_named_args_templates))
            {
              // process named args message when we already have parsed the format message once,
              // and we have the names of each arg cached
              auto const& [message_format, arg_names] = search->second;

              _populate_formatted_log_message(transit_event, message_format.data());
              _populate_formatted_named_args(transit_event, arg_names);
            }
            else
            {
              // process named args log when the message format is processed for the first time
              // parse name of each arg and stored them to our lookup map
              auto const [res_it, inserted] = _named_args_templates.try_emplace(
                _named_args_format_template,
                _process_named_args_format_message(transit_event->macro_metadata->message_format()));

              auto const& [message_format, arg_names] = res_it->second;

              // suppress unused warnings
              (void)inserted;

              _populate_formatted_log_message(transit_event, message_format.data());
              _populate_formatted_named_args(transit_event, arg_names);
            }
          }

          _set_transit_event_mdc(*thread_context, transit_event);
        }
        else if (transit_event->macro_metadata->event() == MacroMetadata::Event::Flush)
        {
          // if this is a flush event then we do not need to format anything for the
          // transit_event, but we need to set the transit event's flush_flag pointer instead
          uintptr_t flush_flag_tmp;
          std::memcpy(&flush_flag_tmp, read_pos, sizeof(uintptr_t));
          transit_event->set_flush_flag(reinterpret_cast<std::atomic<bool>*>(flush_flag_tmp));
          read_pos += sizeof(uintptr_t);
        }
        else if (transit_event->macro_metadata->event() == MacroMetadata::Event::LoggerRemovalRequest)
        {
          // Store the logger name and the sync flag
          uintptr_t logger_removal_flag_tmp;
          std::memcpy(&logger_removal_flag_tmp, read_pos, sizeof(uintptr_t));
          read_pos += sizeof(uintptr_t);
          std::string_view const logger_name = Codec<std::string>::decode_arg(read_pos);

          _logger_removal_flags.emplace(
            std::string{logger_name}, reinterpret_cast<std::atomic<bool>*>(logger_removal_flag_tmp));
        }
        else
        {
          QUILL_ASSERT(
            false,
            "Unhandled event type in BackendWorker::_populate_transit_event_from_frontend_queue()");
        }
      }
    }
    else
    {
      double metric_value;
      std::memcpy(&metric_value, &header_words[3], sizeof(metric_value));
      transit_event->set_metric_value(metric_value);
    }

    if (!is_mdc_event)
    {
      // commit this transit event
      thread_context->_transit_event_buffer->push_back();
    }

    _format_args_store.clear();

    return true;
  }

  /**
   * Checks if there are pending events for caching based on the state of transit event buffers and queues.
   * @return True if there are pending events for caching when the _transit_event_buffer is empty, false otherwise.
   */
  QUILL_ATTRIBUTE_HOT bool has_pending_events_for_caching_when_transit_event_buffer_empty() noexcept
  {
    _update_active_thread_contexts_cache();

    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      QUILL_ASSERT(thread_context->_transit_event_buffer,
                   "transit_event_buffer is nullptr in "
                   "BackendWorker::has_pending_events_for_caching_when_transit_event_buffer_empty()"
                   ", should be valid in _active_thread_contexts_cache");

      if (thread_context->_transit_event_buffer->empty())
      {
        // if there is no _transit_event_buffer yet then check only the queue
        if (thread_context->has_unbounded_queue_type() &&
            !thread_context->get_spsc_queue_union().unbounded_spsc_queue.empty())
        {
          return true;
        }

        if (thread_context->has_bounded_queue_type() &&
            !thread_context->get_spsc_queue_union().bounded_spsc_queue.empty())
        {
          return true;
        }
      }
    }

    return false;
  }

  /**
   * Processes the cached transit event with the minimum timestamp
   */
  QUILL_ATTRIBUTE_HOT bool _process_lowest_timestamp_transit_event()
  {
    // Get the lowest timestamp
    uint64_t min_ts{(std::numeric_limits<uint64_t>::max)()};
    ThreadContext* thread_context{nullptr};

    for (ThreadContext* tc : _active_thread_contexts_cache)
    {
      QUILL_ASSERT(tc->_transit_event_buffer,
                   "transit_event_buffer is nullptr in "
                   "BackendWorker::_process_lowest_timestamp_transit_event(), should be valid in "
                   "_active_thread_contexts_cache");

      TransitEvent const* te = tc->_transit_event_buffer->front();
      if (te && (min_ts > te->timestamp))
      {
        min_ts = te->timestamp;
        thread_context = tc;
      }
    }

    if (!thread_context)
    {
      // all transit event buffers are empty
      return false;
    }

    TransitEvent* transit_event = thread_context->_transit_event_buffer->front();
    QUILL_ASSERT(
      transit_event,
      "transit_event is nullptr in BackendWorker::_process_lowest_timestamp_transit_event() when "
      "transit_buffer is set");

    std::atomic<bool>* flush_flag{nullptr};

    QUILL_TRY { _process_transit_event(*thread_context, *transit_event, flush_flag); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
    QUILL_CATCH_ALL()
    {
      _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
    }
#endif

    // Finally, clean up any remaining fields in the transit event
    if (transit_event->extra_data)
    {
      transit_event->extra_data->named_args.clear();
      transit_event->extra_data->mdc.clear();
      transit_event->extra_data->runtime_metadata.has_runtime_metadata = false;
    }

    // Note: event_payload is reset only in the Flush and Metric branches of _process_transit_event
    // where it is actually populated, rather than unconditionally here. This keeps the common
    // Log path free of an extra variant assignment.

    thread_context->_transit_event_buffer->pop_front();

    if (flush_flag)
    {
      // Process the second part of the flush event after it's been removed from the buffer,
      // ensuring that we are no longer interacting with the thread_context or transit_event.

      // This is particularly important for handling cases when Quill is used as a DLL on Windows.
      // If `FreeLibrary` is called, the backend thread may attempt to access an invalidated
      // `ThreadContext`, which can lead to a crash due to invalid memory access.
      //
      // To prevent this, whenever we receive a Flush event, we clean up any invalidated thread contexts
      // before notifying the caller. This ensures that when `flush_log()` is invoked in `DllMain`
      // during `DLL_PROCESS_DETACH`, the `ThreadContext` is properly cleaned up before the DLL exits.
      _cleanup_invalidated_thread_contexts();

      // Now it’s safe to notify the caller to continue execution.
      flush_flag->store(true);
    }

    return true;
  }

  /**
   * Process a single transit event
   */
  QUILL_ATTRIBUTE_HOT void _process_transit_event(ThreadContext const& thread_context,
                                                  TransitEvent& transit_event, std::atomic<bool>*& flush_flag)
  {
    // If backend_process(...) throws we want to skip this event and move to the next, so we catch
    // the error here instead of catching it in the parent try/catch block of main_loop
    if (transit_event.macro_metadata->event() == MacroMetadata::Event::Log)
    {
      if (transit_event.log_level() != LogLevel::Backtrace)
      {
        _ensure_monotonic_output_timestamp(transit_event);
        _dispatch_transit_event_to_sinks(transit_event, thread_context.thread_id(),
                                         thread_context.thread_name());

        // We also need to check the severity of the log message here against the backtrace
        // Check if we should also flush the backtrace messages:
        // After we forwarded the message we will check the severity of this message for this logger
        // If the severity of the message is higher than the backtrace flush severity we will also
        // flush the backtrace of the logger
        if (QUILL_UNLIKELY(transit_event.log_level() >=
                           transit_event.logger_base->_backtrace_flush_level.load(std::memory_order_relaxed)))
        {
          if (transit_event.logger_base->_backtrace_storage)
          {
            transit_event.logger_base->_backtrace_storage->process(
              [this](TransitEvent const& te, std::string_view thread_id, std::string_view thread_name)
              { _dispatch_transit_event_to_sinks(te, thread_id, thread_name); });
          }
        }
      }
      else
      {
        if (transit_event.logger_base->_backtrace_storage)
        {
          // this is a backtrace log and we will store the transit event
          // we need to pass a copy of transit_event here and not move the existing
          // the transit events are reused
          TransitEvent transit_event_copy;
          transit_event.copy_to(transit_event_copy);

          transit_event.logger_base->_backtrace_storage->store(
            std::move(transit_event_copy), thread_context.thread_id(), thread_context.thread_name());
        }
        else
        {
          QUILL_THROW(
            QuillError{"logger->init_backtrace(...) needs to be called first before using "
                       "LOG_BACKTRACE(...)."});
        }
      }
    }
    else if (transit_event.macro_metadata->event() == MacroMetadata::Event::InitBacktrace)
    {
      // we can just convert the capacity back to int here and use it
      if (!transit_event.logger_base->_backtrace_storage)
      {
        // Lazy BacktraceStorage creation
        transit_event.logger_base->_backtrace_storage = std::make_shared<BacktraceStorage>();
      }

      transit_event.logger_base->_backtrace_storage->set_capacity(static_cast<uint32_t>(std::stoul(
        std::string{transit_event.formatted_msg->begin(), transit_event.formatted_msg->end()})));
    }
    else if (transit_event.macro_metadata->event() == MacroMetadata::Event::FlushBacktrace)
    {
      if (transit_event.logger_base->_backtrace_storage)
      {
        // process all records in backtrace for this logger and log them
        transit_event.logger_base->_backtrace_storage->process(
          [this](TransitEvent const& te, std::string_view thread_id, std::string_view thread_name)
          { _dispatch_transit_event_to_sinks(te, thread_id, thread_name); });
      }
    }
    else if (transit_event.macro_metadata->event() == MacroMetadata::Event::Flush)
    {
      _flush_and_run_active_sinks(false, std::chrono::milliseconds{0});

      // This is a flush event, so we capture the flush flag to notify the caller after processing.
      flush_flag = transit_event.flush_flag();

      // Reset the flush flag as TransitEvents are re-used, preventing incorrect flag reuse.
      transit_event.reset_payload();

      // We defer notifying the caller until after this function completes.
    }
    else if (transit_event.macro_metadata->event() == MacroMetadata::Event::Metric)
    {
      _ensure_monotonic_output_timestamp(transit_event);
      _write_metric_sample(transit_event, thread_context.thread_id(), thread_context.thread_name());

      // Reset the payload as TransitEvents are re-used, so a later reuse of this slot starts
      // from a clean variant state instead of still carrying the metric value.
      transit_event.reset_payload();
    }
    else
    {
      QUILL_ASSERT(transit_event.macro_metadata->event() == MacroMetadata::Event::LoggerRemovalRequest,
                   "Unexpected transit event type in BackendWorker::_process_transit_event()");
      // LoggerRemovalRequest is handled when the removal flag map is populated. It has no sink
      // dispatch work of its own.
    }
  }

  /**
   * Adjusts regular output event timestamps so sink-visible timestamps do not move backwards when
   * the feature is enabled. This runs only after an event has been selected for processing, so the
   * transit event buffers are still ordered by original timestamp.
   */
  QUILL_ATTRIBUTE_HOT void _ensure_monotonic_output_timestamp(TransitEvent& transit_event) noexcept
  {
    if (!_options.ensure_monotonic_output_timestamps)
    {
      return;
    }

    if (QUILL_UNLIKELY(transit_event.timestamp < _last_output_timestamp))
    {
      transit_event.timestamp = (_last_output_timestamp == (std::numeric_limits<uint64_t>::max)())
        ? _last_output_timestamp
        : _last_output_timestamp + 1;
    }

    _last_output_timestamp = transit_event.timestamp;
  }

  /**
   * Dispatches a transit event
   */
  QUILL_ATTRIBUTE_HOT void _dispatch_transit_event_to_sinks(TransitEvent const& transit_event,
                                                            std::string_view thread_id, std::string_view thread_name)
  {
    // First check to see if we should init the pattern formatter on a new Logger
    // Look up to see if we have the formatter and if not create it
    if (QUILL_UNLIKELY(!transit_event.logger_base->_pattern_formatter))
    {
      // Search for an existing pattern_formatter in each logger
      _logger_manager.for_each_logger(
        [&transit_event](LoggerBase* logger)
        {
          if (logger->_pattern_formatter &&
              (logger->_pattern_formatter->get_options() == transit_event.logger_base->_pattern_formatter_options))
          {
            // hold a copy of the shared_ptr of the same formatter
            transit_event.logger_base->_pattern_formatter = logger->_pattern_formatter;
            return true;
          }

          return false;
        });

      if (!transit_event.logger_base->_pattern_formatter)
      {
        // Didn't find an existing formatter  need to create a new pattern formatter
        transit_event.logger_base->_pattern_formatter =
          std::make_shared<PatternFormatter>(transit_event.logger_base->_pattern_formatter_options);
      }
    }

    QUILL_ASSERT(transit_event.logger_base->_pattern_formatter,
                 "pattern_formatter is nullptr in BackendWorker::_process_transit_event(), should "
                 "have been created earlier");

    // proceed after ensuring a pattern formatter exists
    std::string_view const log_level_description =
      log_level_to_string(transit_event.log_level(), _options.log_level_descriptions.data(),
                          _options.log_level_descriptions.size());

    std::string_view const log_level_short_code =
      log_level_to_string(transit_event.log_level(), _options.log_level_short_codes.data(),
                          _options.log_level_short_codes.size());

    _write_log_statement(
      transit_event, thread_id, thread_name, log_level_description, log_level_short_code,
      std::string_view{transit_event.formatted_msg->data(), transit_event.formatted_msg->size()});
  }

  /**
   * Forwards a decoded metric sample to each sink associated with the logger.
   */
  QUILL_ATTRIBUTE_HOT void _write_metric_sample(TransitEvent const& transit_event, std::string_view thread_id,
                                                std::string_view thread_name) const
  {
    QUILL_ASSERT(
      transit_event.macro_metadata,
      "transit_event.macro_metadata is nullptr in BackendWorker::_write_metric_sample()");
    QUILL_ASSERT(transit_event.macro_metadata->event() == MacroMetadata::Event::Metric,
                 "Unexpected transit event type in BackendWorker::_write_metric_sample()");

    // MacroMetadata is the first non-virtual base of MetricMetadata so static_cast is well-defined
    // here. The Event::Metric check above guarantees the dynamic type.
    auto const* metric_metadata = static_cast<MetricMetadata const*>(transit_event.macro_metadata);
    double const metric_value = transit_event.metric_value();

    for (auto& sink : transit_event.logger_base->_sinks)
    {
      QUILL_TRY
      {
        sink->write_metric(metric_metadata, transit_event.timestamp, thread_id, thread_name,
                           _process_id, transit_event.logger_base->_logger_name, metric_value);
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
      QUILL_CATCH_ALL()
      {
        _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
      }
#endif
    }
  }

  /**
   * Formats and writes the log statement to each sink
   */
  QUILL_ATTRIBUTE_HOT void _write_log_statement(TransitEvent const& transit_event,
                                                std::string_view const& thread_id,
                                                std::string_view const& thread_name,
                                                std::string_view const& log_level_description,
                                                std::string_view const& log_level_short_code,
                                                std::string_view const& log_message) const
  {
    std::string_view default_log_statement;

    // Process each sink with the appropriate formatting and filtering
    for (auto& sink : transit_event.logger_base->_sinks)
    {
      QUILL_TRY
      {
        std::string_view log_to_write;

        // Determine which formatted log to use
        if (!sink->_override_pattern_formatter_options)
        {
          if (default_log_statement.empty())
          {
            // Use the default formatted log statement, here by checking empty() we try to format
            // once even for multiple sinks
            default_log_statement = transit_event.logger_base->_pattern_formatter->format(
              transit_event.timestamp, thread_id, thread_name, _process_id,
              transit_event.logger_base->_logger_name, log_level_description, log_level_short_code,
              *transit_event.macro_metadata, transit_event.get_named_args(), log_message,
              transit_event.mdc());
          }

          log_to_write = default_log_statement;
        }
        else
        {
          // Sink has override_pattern_formatter_options, we do not include PatternFormatter
          // in the frontend fo this reason we init PatternFormatter here
          if (!sink->_override_pattern_formatter)
          {
            // Initialize override formatter if needed
            sink->_override_pattern_formatter =
              std::make_shared<PatternFormatter>(*sink->_override_pattern_formatter_options);
          }

          // Use the sink's override formatter
          log_to_write = sink->_override_pattern_formatter->format(
            transit_event.timestamp, thread_id, thread_name, _process_id, transit_event.logger_base->_logger_name,
            log_level_description, log_level_short_code, *transit_event.macro_metadata,
            transit_event.get_named_args(), log_message, transit_event.mdc());
        }

        // Apply filters now that we have the formatted log
        if (sink->apply_all_filters(transit_event.macro_metadata, transit_event.timestamp,
                                    thread_id, thread_name, transit_event.logger_base->_logger_name,
                                    transit_event.log_level(), log_message, log_to_write))
        {
          // Forward the message using the computed log statement that passed the filter
          sink->write_log(transit_event.macro_metadata, transit_event.timestamp, thread_id,
                          thread_name, _process_id, transit_event.logger_base->_logger_name,
                          transit_event.log_level(), log_level_description, log_level_short_code,
                          transit_event.get_named_args(), log_message, log_to_write);
        }
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
      QUILL_CATCH_ALL()
      {
        _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
      }
#endif
    }
  }

  /**
   * Check for dropped or blocked events
   * @param error_notifier error notifier
   */
  QUILL_ATTRIBUTE_HOT void _check_failure_counter(std::function<void(std::string const&)> const& error_notifier)
  {
    if (!error_notifier)
    {
      return;
    }

    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      size_t const failed_events_cnt = thread_context->get_and_reset_failure_counter();

      if (QUILL_UNLIKELY(failed_events_cnt > 0))
      {
        std::string const timestamp = _get_local_time_str();

        if (thread_context->has_dropping_queue())
        {
          if (thread_context->has_unbounded_queue_type())
          {
            _notify_error(error_notifier,
                          fmtquill::format("{} Quill INFO: Reached the maximum configured "
                                           "unbounded queue capacity and dropped "
                                           "{} events from thread {}",
                                           timestamp, failed_events_cnt, thread_context->thread_id()));
          }
          else
          {
            _notify_error(error_notifier,
                          fmtquill::format("{} Quill INFO: Dropped {} events from thread {}",
                                           timestamp, failed_events_cnt, thread_context->thread_id()));
          }
        }
        else if (thread_context->has_blocking_queue())
        {
          if (thread_context->has_unbounded_queue_type())
          {
            _notify_error(
              error_notifier,
              fmtquill::format(
                "{} Quill INFO: Reached the maximum configured unbounded queue capacity and "
                "experienced {} blocking occurrences on thread {}",
                timestamp, failed_events_cnt, thread_context->thread_id()));
          }
          else
          {
            _notify_error(
              error_notifier,
              fmtquill::format("{} Quill INFO: Experienced {} blocking occurrences on thread {}",
                               timestamp, failed_events_cnt, thread_context->thread_id()));
          }
        }
      }
    }
  }

  /**
   * Process the format of a log message that contains named args
   * @param fmt_template a log message containing named arguments
   * @return first: fmt string without the named arguments, second: a vector extracted keys
   */
  QUILL_ATTRIBUTE_HOT static std::pair<std::string, std::vector<std::pair<std::string, std::string>>> _process_named_args_format_message(
    std::string_view fmt_template)
  {
    // It would be nice to do this at compile time and store it in macro metadata, but without
    // constexpr vector and string in c++17 it is not possible
    std::string fmt_str;
    std::vector<std::pair<std::string, std::string>> keys;
    fmt_str.reserve(fmt_template.size());

    size_t pos = 0;
    while (pos < fmt_template.size())
    {
      if (fmt_template[pos] != '{')
      {
        fmt_str += fmt_template[pos];
        ++pos;
        continue;
      }

      if ((pos + 1 < fmt_template.size()) && (fmt_template[pos + 1] == '{'))
      {
        fmt_str += "{{";
        pos += 2;
        continue;
      }

      size_t const placeholder_start = pos;
      ++pos; // consume '{'

      size_t const close_bracket_pos = _find_placeholder_closing_brace(fmt_template, pos);
      if (close_bracket_pos == std::string_view::npos)
      {
        fmt_str += std::string{fmt_template.substr(placeholder_start)};
        break;
      }

      std::string_view const text_inside_placeholders = fmt_template.substr(pos, close_bracket_pos - pos);
      std::string_view arg_syntax;
      std::string_view arg_name;

      if (size_t const syntax_separator = text_inside_placeholders.find(':');
          syntax_separator != std::string_view::npos)
      {
        arg_syntax = text_inside_placeholders.substr(
          syntax_separator, text_inside_placeholders.size() - syntax_separator);
        arg_name = text_inside_placeholders.substr(0, syntax_separator);
      }
      else
      {
        arg_name = text_inside_placeholders;
      }

      std::string const normalized_arg_syntax = _normalize_named_arg_syntax(arg_syntax);

      fmt_str += "{";
      fmt_str += normalized_arg_syntax;
      fmt_str += "}";

      keys.emplace_back(_make_unique_named_arg_key(keys, arg_name), normalized_arg_syntax);
      pos = close_bracket_pos + 1;
    }

    return std::make_pair(fmt_str, keys);
  }

  QUILL_NODISCARD static std::string _make_unique_named_arg_key(
    std::vector<std::pair<std::string, std::string>> const& existing_keys, std::string_view arg_name)
  {
    std::string key{arg_name};

    for (size_t suffix = 1;; ++suffix)
    {
      bool key_already_exists = false;
      for (auto const& existing_key : existing_keys)
      {
        if (existing_key.first == key)
        {
          key_already_exists = true;
          break;
        }
      }

      if (!key_already_exists)
      {
        return key;
      }

      key = std::string{arg_name};
      key += "_";
      key += std::to_string(suffix);
    }
  }

  QUILL_NODISCARD static size_t _find_placeholder_closing_brace(std::string_view text, size_t pos) noexcept
  {
    size_t nested_replacement_fields = 0;

    while (pos < text.size())
    {
      if (text[pos] == '{')
      {
        if ((pos + 1 < text.size()) && (text[pos + 1] == '{'))
        {
          pos += 2;
          continue;
        }

        ++nested_replacement_fields;
        ++pos;
        continue;
      }

      if (text[pos] == '}')
      {
        if (nested_replacement_fields == 0)
        {
          return pos;
        }

        --nested_replacement_fields;
      }

      ++pos;
    }

    return std::string_view::npos;
  }

  QUILL_NODISCARD static std::string _normalize_named_arg_syntax(std::string_view arg_syntax)
  {
    std::string normalized;
    normalized.reserve(arg_syntax.size());

    size_t pos = 0;
    while (pos < arg_syntax.size())
    {
      if (arg_syntax[pos] != '{')
      {
        normalized += arg_syntax[pos];
        ++pos;
        continue;
      }

      if ((pos + 1 < arg_syntax.size()) && (arg_syntax[pos + 1] == '{'))
      {
        normalized += "{{";
        pos += 2;
        continue;
      }

      size_t const close_bracket_pos = _find_placeholder_closing_brace(arg_syntax, pos + 1);
      if (close_bracket_pos == std::string_view::npos)
      {
        normalized += std::string{arg_syntax.substr(pos)};
        break;
      }

      normalized += "{}";
      pos = close_bracket_pos + 1;
    }

    return normalized;
  }

  /**
   * Helper function to read the unbounded queue and also report the allocation
   * @param frontend_queue queue
   * @param thread_context thread context
   * @return start position of read
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* _read_unbounded_frontend_queue(UnboundedSPSCQueue& frontend_queue,
                                                                                ThreadContext* thread_context) const
  {
    auto const read_result = frontend_queue.prepare_read();

    if (read_result.allocation)
    {
      if ((read_result.new_capacity < read_result.previous_capacity) && thread_context->_transit_event_buffer)
      {
        // The user explicitly requested to shrink the queue, indicating a preference for low memory
        // usage. To align with this intent, we also request shrinking the backend buffer.
        thread_context->_transit_event_buffer->request_shrink();
      }

      // When allocation_info has a value it means that the queue has re-allocated
      if (_options.error_notifier)
      {
        // we switched to a new here, and we also notify the user of the allocation via the
        // error_notifier
        _notify_error(
          _options.error_notifier,
          fmtquill::format("{} Quill INFO: Allocated a new SPSC queue with a capacity of {} KiB "
                           "(previously {} KiB) from thread {}",
                           _get_local_time_str(), (read_result.new_capacity / 1024),
                           (read_result.previous_capacity / 1024), thread_context->thread_id()));
      }
    }

    return read_result.read_pos;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool _check_frontend_queues_and_cached_transit_events_empty()
  {
    _update_active_thread_contexts_cache();

    bool all_empty{true};

    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      QUILL_ASSERT(thread_context->has_unbounded_queue_type() || thread_context->has_bounded_queue_type(),
                   "Invalid queue type in "
                   "BackendWorker::_check_frontend_queues_and_cached_transit_events_empty()");

      if (thread_context->has_unbounded_queue_type())
      {
        all_empty &= thread_context->get_spsc_queue_union().unbounded_spsc_queue.empty();
      }
      else if (thread_context->has_bounded_queue_type())
      {
        all_empty &= thread_context->get_spsc_queue_union().bounded_spsc_queue.empty();
      }

      QUILL_ASSERT(thread_context->_transit_event_buffer,
                   "transit_event_buffer is nullptr in "
                   "BackendWorker::_check_frontend_queues_and_cached_transit_events_empty(), "
                   "should be valid in _active_thread_contexts_cache");

      all_empty &= thread_context->_transit_event_buffer->empty();
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
      if (auto const now = std::chrono::steady_clock::now();
          (now - _last_rdtsc_resync_time) > _options.rdtsc_resync_interval)
      {
        if (_rdtsc_clock.load(std::memory_order_relaxed)->resync(2500))
        {
          _last_rdtsc_resync_time = now;
        }
      }
    }
  }

  /***/
  QUILL_ATTRIBUTE_HOT void _flush_and_run_active_sinks(bool run_periodic_tasks, std::chrono::milliseconds sink_min_flush_interval)
  {
    // Populate the active sinks cache with unique sinks, consider only the valid loggers
    _logger_manager.for_each_logger(
      [this](LoggerBase* logger)
      {
        if (logger->is_valid_logger())
        {
          for (std::shared_ptr<Sink> const& sink : logger->_sinks)
          {
            Sink* logger_sink_ptr = sink.get();
            auto search_it = std::find_if(_active_sinks_cache.begin(), _active_sinks_cache.end(),
                                          [logger_sink_ptr](Sink* elem)
                                          {
                                            // no one else can remove the shared pointer as this is
                                            // only running on backend thread
                                            return elem == logger_sink_ptr;
                                          });

            if (search_it == std::end(_active_sinks_cache))
            {
              _active_sinks_cache.push_back(logger_sink_ptr);
            }
          }
        }

        // return false to never end the loop early
        return false;
      });

    bool should_flush_sinks{false};
    std::chrono::steady_clock::time_point now;

    if (sink_min_flush_interval.count())
    {
      // conditional flush sinks
      now = std::chrono::steady_clock::now();
      if ((now - _last_sink_flush_time) > sink_min_flush_interval)
      {
        should_flush_sinks = true;
      }
    }
    else
    {
      // sink_min_flush_interval == 0 - always flush sinks
      should_flush_sinks = true;
    }

    for (auto const& sink : _active_sinks_cache)
    {
      QUILL_TRY
      {
        if (should_flush_sinks)
        {
          // If an exception is thrown, catch it here to prevent it from propagating
          // to the outer function. This prevents potential infinite loops caused by failing
          // flush operations.
          sink->flush_sink();
        }
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
      QUILL_CATCH_ALL()
      {
        _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
      }
#endif

      if (run_periodic_tasks)
      {
        QUILL_TRY { sink->run_periodic_tasks(); }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
        QUILL_CATCH_ALL()
        {
          _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
        }
#endif
      }
    }

    // Only update the timestamp after we actually attempted to flush
    if (should_flush_sinks && sink_min_flush_interval.count() && !_active_sinks_cache.empty())
    {
      _last_sink_flush_time = now;
    }

    _active_sinks_cache.clear();
  }

  /**
   * Reloads the thread contexts in our local cache.
   */
  QUILL_ATTRIBUTE_HOT void _update_active_thread_contexts_cache(bool force = false)
  {
    // Check if _thread_contexts has changed. This can happen only when a new thread context is added by any Logger
    if (QUILL_UNLIKELY(force || _thread_context_manager.new_thread_context_flag()))
    {
      _active_thread_contexts_cache.clear();
      _thread_context_manager.for_each_thread_context(
        [this](ThreadContext* thread_context)
        {
          if (!thread_context->_transit_event_buffer)
          {
            // Lazy initialise the _transit_event_buffer for this thread_context
            thread_context->_transit_event_buffer =
              std::make_shared<TransitEventBuffer>(_options.transit_event_buffer_initial_capacity);
          }

          // We do not skip invalidated && empty queue thread contexts as this is very rare,
          // so instead we just add them and expect them to be cleaned in the next iteration
          _active_thread_contexts_cache.push_back(thread_context);
        });
    }
  }

  /**
   * Looks into the _thread_context_cache and removes all thread contexts that are 1) invalidated
   * and 2) have an empty frontend queue and no cached transit events to process
   *
   * @note Only called by the backend thread
   */
  QUILL_ATTRIBUTE_HOT void _cleanup_invalidated_thread_contexts()
  {
    if (!_thread_context_manager.has_invalid_thread_context())
    {
      return;
    }

    auto find_invalid_and_empty_thread_context_callback = [](ThreadContext* thread_context)
    {
      // If the thread context is invalid it means the thread that created it has now died.
      // We also want to empty the queue from all LogRecords before removing the thread context
      if (!thread_context->is_valid())
      {
        QUILL_ASSERT(thread_context->has_unbounded_queue_type() || thread_context->has_bounded_queue_type(),
                     "Invalid queue type in BackendWorker::_update_active_thread_contexts_cache()");

        QUILL_ASSERT(thread_context->_transit_event_buffer,
                     "transit_event_buffer is nullptr in "
                     "BackendWorker::_update_active_thread_contexts_cache(), should be valid in "
                     "_active_thread_contexts_cache");

        // detect empty queue
        if (thread_context->has_unbounded_queue_type())
        {
          return thread_context->get_spsc_queue_union().unbounded_spsc_queue.empty() &&
            thread_context->_transit_event_buffer->empty();
        }

        if (thread_context->has_bounded_queue_type())
        {
          return thread_context->get_spsc_queue_union().bounded_spsc_queue.empty() &&
            thread_context->_transit_event_buffer->empty();
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
   * Cleans up any invalidated loggers
   */
  QUILL_ATTRIBUTE_HOT void _cleanup_invalidated_loggers()
  {
    // since there are no messages we can check for invalidated loggers and clean them up
    _logger_manager.cleanup_invalidated_loggers(
      [this]()
      {
        // check the queues are empty each time before removing a logger to avoid
        // potential race condition of the logger* still being in the queue
        return _check_frontend_queues_and_cached_transit_events_empty();
      },
      _removed_loggers);

    if (!_removed_loggers.empty())
    {
      // if loggers were removed also check for sinks to remove
      // cleanup_unused_sinks is expensive and should be only called when it is needed
      _sink_manager.cleanup_unused_sinks();

      for (auto const& removed_logger_name : _removed_loggers)
      {
        // Notify the user if the blocking call was used
        auto search_it = _logger_removal_flags.find(removed_logger_name);
        if (search_it != _logger_removal_flags.end())
        {
          search_it->second->store(true);
          _logger_removal_flags.erase(search_it);
        }
      }

      _removed_loggers.clear();
    }
  }

  /**
   * Shrinks empty TransitEvent buffers. This is triggered only when the user explicitly
   * requests shrinking of the unbounded frontend queue to optimize memory usage.
   */
  QUILL_ATTRIBUTE_HOT void _try_shrink_empty_transit_event_buffers()
  {
    for (ThreadContext* thread_context : _active_thread_contexts_cache)
    {
      if (thread_context->_transit_event_buffer)
      {
        thread_context->_transit_event_buffer->try_shrink();
      }
    }
  }

  void _apply_mdc_event(ThreadContext& thread_context, MacroMetadata::Event event,
                        FormatArgsDecoder format_args_decoder, std::byte*& read_pos)
  {
    if (event == MacroMetadata::Event::MdcSet)
    {
      format_args_decoder(read_pos, _format_args_store);

      QUILL_ASSERT((_format_args_store.size() % 2) == 0,
                   "MdcSet decoded argument count mismatch in BackendWorker::_apply_mdc_event()");

      if (!thread_context._backend_mdc_state)
      {
        thread_context._backend_mdc_state = std::make_shared<BackendMdcState>(_options.mdc_format_pattern);
      }

      BackendMdcState& mdc_state = *thread_context._backend_mdc_state;
      auto const field_count = static_cast<uint32_t>(_format_args_store.size() / 2);

      _mdc_fields.clear();
      _mdc_fields.reserve(field_count);

      QUILL_TRY
      {
        for (uint32_t i = 0; i < field_count; ++i)
        {
          int const key_index = static_cast<int>(i * 2u);
          int const value_index = key_index + 1;

          std::string const key = fmtquill::vformat(
            "{}", fmtquill::basic_format_args<fmtquill::format_context>{_format_args_store.data() + key_index, 1});
          std::string const value = fmtquill::vformat(
            "{}", fmtquill::basic_format_args<fmtquill::format_context>{_format_args_store.data() + value_index, 1});

          _mdc_fields.emplace_back(key, value);
        }
      }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e)
      {
        _notify_error(_options.error_notifier, e.what());
        return;
      }
      QUILL_CATCH_ALL()
      {
        _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
        return;
      }
#endif

      for (auto const& [key, value] : _mdc_fields)
      {
        mdc_state.set(key, value);
      }

      QUILL_TRY { mdc_state.rebuild_formatted_mdc(); }
#if !defined(QUILL_NO_EXCEPTIONS)
      QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
      QUILL_CATCH_ALL()
      {
        _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
      }
#endif
    }
    else if (event == MacroMetadata::Event::MdcErase)
    {
      format_args_decoder(read_pos, _format_args_store);

      QUILL_ASSERT(_format_args_store.size() >= 1,
                   "MdcErase decoded argument count mismatch in BackendWorker::_apply_mdc_event()");

      if (thread_context._backend_mdc_state)
      {
        _mdc_keys.clear();
        _mdc_keys.reserve(static_cast<size_t>(_format_args_store.size()));

        QUILL_TRY
        {
          for (int i = 0; i < _format_args_store.size(); ++i)
          {
            _mdc_keys.emplace_back(fmtquill::vformat(
              "{}", fmtquill::basic_format_args<fmtquill::format_context>{_format_args_store.data() + i, 1}));
          }
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(std::exception const& e)
        {
          _notify_error(_options.error_notifier, e.what());
          return;
        }
        QUILL_CATCH_ALL()
        {
          _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
          return;
        }
#endif

        for (std::string const& key : _mdc_keys)
        {
          thread_context._backend_mdc_state->erase(key);
        }

        if (thread_context._backend_mdc_state->empty())
        {
          thread_context._backend_mdc_state.reset();
        }
        else
        {
          QUILL_TRY { thread_context._backend_mdc_state->rebuild_formatted_mdc(); }
#if !defined(QUILL_NO_EXCEPTIONS)
          QUILL_CATCH(std::exception const& e) { _notify_error(_options.error_notifier, e.what()); }
          QUILL_CATCH_ALL()
          {
            _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
          }
#endif
        }
      }
    }
    else if (event == MacroMetadata::Event::MdcClear)
    {
      thread_context._backend_mdc_state.reset();
    }
    else
    {
      QUILL_ASSERT(false, "Unexpected MDC event in BackendWorker::_apply_mdc_event()");
    }
  }

  void _set_transit_event_mdc(ThreadContext const& thread_context, TransitEvent* transit_event)
  {
    if (thread_context._backend_mdc_state)
    {
      std::string_view const mdc = thread_context._backend_mdc_state->formatted_mdc();
      transit_event->ensure_extra_data();
      transit_event->extra_data->mdc.assign(mdc.data(), mdc.size());
    }
  }

  /** Formats each named-arg value individually using the corresponding argument tail. */
  static void _format_and_split_arguments(std::vector<std::pair<std::string, std::string>> const& orig_arg_names,
                                          std::vector<std::pair<std::string, std::string>>& named_args,
                                          DynamicFormatArgStore const& format_args_store,
                                          BackendOptions const& options)
  {
    for (size_t i = 0; i < named_args.size(); ++i)
    {
      if (i >= static_cast<size_t>(format_args_store.size()))
      {
        break;
      }

      std::string format_string{"{}"};

      // orig_arg_names[i].second stores the normalized format syntax, for example ":.2f" or ":{}".
      if ((i < orig_arg_names.size()) && !orig_arg_names[i].second.empty())
      {
        format_string = "{";
        format_string += orig_arg_names[i].second;
        format_string += "}";
      }

      fmtquill::vformat_to(std::back_inserter(named_args[i].second), format_string,
                           fmtquill::basic_format_args<fmtquill::format_context>{
                             format_args_store.data() + static_cast<std::ptrdiff_t>(i),
                             format_args_store.size() - static_cast<int>(i)});

      if (options.check_printable_char && format_args_store.has_string_related_type())
      {
        sanitize_non_printable_chars(named_args[i].second, options);
      }
    }
  }

  void _populate_formatted_named_args(TransitEvent* transit_event,
                                      std::vector<std::pair<std::string, std::string>> const& arg_names)
  {
    transit_event->ensure_extra_data();

    auto* named_args = &transit_event->extra_data->named_args;
    named_args->clear();

    if (arg_names.empty())
    {
      return;
    }

    named_args->resize(arg_names.size());

    // We first populate the arg names in the transit buffer
    for (size_t i = 0; i < arg_names.size(); ++i)
    {
      (*named_args)[i].first = arg_names[i].first;
    }

    for (size_t i = arg_names.size(); i < static_cast<size_t>(_format_args_store.size()); ++i)
    {
      // we do not have a named_arg for the argument value here so we just append its index as a placeholder
      named_args->push_back(std::pair<std::string, std::string>(fmtquill::format("_{}", i), std::string{}));
    }

    // Then populate all the values of each arg
    QUILL_TRY { _format_and_split_arguments(arg_names, *named_args, _format_args_store, _options); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH(std::exception const&)
    {
      // This catch block simply catches the exception.
      // Since the error has already been handled in _populate_formatted_log_message,
      // there is no additional action required here.
    }
    QUILL_CATCH_ALL()
    {
      _notify_error(_options.error_notifier, std::string{"Caught unhandled exception."});
    }
#endif
  }

  QUILL_ATTRIBUTE_HOT void _populate_formatted_log_message(TransitEvent* transit_event, char const* message_format)
  {
    transit_event->formatted_msg->clear();

    QUILL_TRY
    {
      fmtquill::vformat_to(std::back_inserter(*transit_event->formatted_msg), message_format,
                           fmtquill::basic_format_args<fmtquill::format_context>{
                             _format_args_store.data(), _format_args_store.size()});

      if (_options.check_printable_char && _format_args_store.has_string_related_type())
      {
        sanitize_non_printable_chars(*transit_event->formatted_msg, _options);
      }
    }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH(std::exception const& e)
    {
      transit_event->formatted_msg->clear();
      std::string const error =
        fmtquill::format(R"([Could not format log statement. message: "{}", location: "{}", error: "{}"])",
                         transit_event->macro_metadata->message_format(),
                         transit_event->macro_metadata->short_source_location(), e.what());

      transit_event->formatted_msg->append(error);
      _notify_error(_options.error_notifier, error);
    }
    QUILL_CATCH_ALL()
    {
      transit_event->formatted_msg->clear();
      std::string const error = fmtquill::format(
        R"([Could not format log statement. message: "{}", location: "{}", error: "{}"])",
        transit_event->macro_metadata->message_format(),
        transit_event->macro_metadata->short_source_location(), "Caught unhandled exception.");

      transit_event->formatted_msg->append(error);
      _notify_error(_options.error_notifier, error);
    }
#endif
  }

  void _apply_runtime_metadata(std::byte*& read_pos, TransitEvent* transit_event)
  {
    char const* fmt;
    char const* file;
    char const* function;
    char const* tags;

    if (transit_event->macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataDeepCopy)
    {
      fmt = Codec<char const*>::decode_arg(read_pos);
      file = Codec<char const*>::decode_arg(read_pos);
      function = Codec<char const*>::decode_arg(read_pos);
      tags = Codec<char const*>::decode_arg(read_pos);
    }
    else if (transit_event->macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataShallowCopy)
    {
      fmt = static_cast<char const*>(Codec<void const*>::decode_arg(read_pos));
      file = static_cast<char const*>(Codec<void const*>::decode_arg(read_pos));
      function = static_cast<char const*>(Codec<void const*>::decode_arg(read_pos));
      tags = static_cast<char const*>(Codec<void const*>::decode_arg(read_pos));
    }
    else if (transit_event->macro_metadata->event() == MacroMetadata::Event::LogWithRuntimeMetadataHybridCopy)
    {
      fmt = Codec<char const*>::decode_arg(read_pos);
      file = static_cast<char const*>(Codec<void const*>::decode_arg(read_pos));
      function = static_cast<char const*>(Codec<void const*>::decode_arg(read_pos));
      tags = Codec<char const*>::decode_arg(read_pos);
    }
    else
    {
      QUILL_THROW(
        QuillError{"Unexpected event type in _apply_runtime_metadata. This should never happen."});
    }

    auto const line = Codec<uint32_t>::decode_arg(read_pos);
    auto const log_level = Codec<LogLevel>::decode_arg(read_pos);

    auto temp = TransitEvent::RuntimeMetadata{file, line, function, tags, fmt, log_level};

    transit_event->ensure_extra_data();
    transit_event->extra_data->runtime_metadata = temp;

    // point to the runtime metadata
    transit_event->macro_metadata = &transit_event->extra_data->runtime_metadata.macro_metadata;
  }

  template <typename TFormattedMsg>
  static void sanitize_non_printable_chars(TFormattedMsg& formatted_msg, BackendOptions const& options)
  {
    // check for non-printable characters in the formatted_msg
    bool contains_non_printable_char{false};

    for (char c : formatted_msg)
    {
      if (!options.check_printable_char(c))
      {
        contains_non_printable_char = true;
        break;
      }
    }

    if (contains_non_printable_char)
    {
      // in this rare event we will replace the non-printable chars with their hex values
      std::string const formatted_msg_copy = {formatted_msg.data(), formatted_msg.size()};
      formatted_msg.clear();

      for (char c : formatted_msg_copy)
      {
        if (options.check_printable_char(c))
        {
          formatted_msg.push_back(c);
        }
        else
        {
          // convert non-printable character to hex
          unsigned char const byte = static_cast<unsigned char>(c);
          constexpr char hex[] = "0123456789ABCDEF";
          formatted_msg.push_back('\\');
          formatted_msg.push_back('x');
          formatted_msg.push_back(hex[(byte >> 4) & 0xF]);
          formatted_msg.push_back(hex[byte & 0xF]);
        }
      }
    }
  }

private:
  friend class quill::ManualBackendWorker;

  std::unique_ptr<BackendWorkerLock> _backend_worker_lock;
  ThreadContextManager& _thread_context_manager = ThreadContextManager::instance();
  SinkManager& _sink_manager = SinkManager::instance();
  LoggerManager& _logger_manager = LoggerManager::instance();
  BackendOptions _options;
  uint64_t _last_output_timestamp{0};
  std::thread _worker_thread;

  DynamicFormatArgStore _format_args_store; /** Format args tmp storage as member to avoid reallocation */
  std::vector<std::string> _removed_loggers; /** Even an empty vector causes an allocation on Windows */
  std::vector<ThreadContext*> _active_thread_contexts_cache;
  std::vector<Sink*> _active_sinks_cache; /** Member to avoid re-allocating **/
  std::vector<std::pair<std::string, std::string>> _mdc_fields; /** MDC set scratch storage */
  std::vector<std::string> _mdc_keys;                           /** MDC erase scratch storage */
  std::unordered_map<std::string, std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> _named_args_templates; /** Avoid re-formating the same named args log template each time */
  std::unordered_map<std::string, std::atomic<bool>*> _logger_removal_flags; /** Maps logger names to atomic flags used for synchronizing remove_logger_blocking(). */
  std::string _named_args_format_template; /** to avoid allocation each time **/
  std::string _process_id;                 /** Id of the current running process **/
  std::chrono::steady_clock::time_point _last_rdtsc_resync_time;
  std::chrono::steady_clock::time_point _last_sink_flush_time;
  std::atomic<uint32_t> _worker_thread_id{0};  /** cached backend worker thread id */
  std::atomic<bool> _is_worker_running{false}; /** The spawned backend thread status */
  std::atomic<bool> _has_worker_thread_exited{true}; /** Set to true when the backend thread completes its exit sequence */

  alignas(QUILL_CACHE_LINE_ALIGNED) std::atomic<RdtscClock*> _rdtsc_clock{
    nullptr}; /** rdtsc clock if enabled, can be accessed by any thread **/
  alignas(QUILL_CACHE_LINE_ALIGNED) std::mutex _wake_up_mutex;
  std::condition_variable _wake_up_cv;
  bool _wake_up_flag{false};
};

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif

} // namespace detail

QUILL_END_NAMESPACE
