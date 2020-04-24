/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/Config.h"
#include "quill/detail/HandlerCollection.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/misc/RdtscClock.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace quill
{
namespace detail
{

class BackendWorker
{
public:
  /**
   * Constructor
   */
  BackendWorker(Config const& config,
                ThreadContextCollection& thread_context_collection,
                HandlerCollection const& handler_collection);

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
   * Starts the backend worker thread
   * @throws std::runtime_error, std::system_error on failures
   */
  QUILL_ATTRIBUTE_COLD inline void run();

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop() noexcept;

private:
  /**
   * Backend worker thread main function
   */
  QUILL_ATTRIBUTE_HOT inline void _main_loop();

  /**
   * Logging thread exist function that flushes everything after stop() is called
   */
  QUILL_ATTRIBUTE_COLD void _exit();

  /**
   * Checks for records in all queues and processes the one with the minimum timestamp
   * @return true if one record was found and processed
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline bool _process_record(
    ThreadContextCollection::backend_thread_contexts_cache_t const& thread_contexts);

  /**
   * Convert a log record timestamp to a time since epoch timestamp in nanoseconds.
   *
   * @param log_record_timestamp The log record timestamp is just an uint64 and it can be either rdtsc time or nanoseconds since epoch based QUILL_RDTSC_CLOCK defintion
   * @return a timestamp in nanoseconds since epoch
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::chrono::nanoseconds _get_real_timestamp(
    ThreadContext::SPSCQueueT::handle_t const& log_record_handle) const noexcept;

private:
  /** This is exactly 1 cache line **/
  Config const& _config;
  ThreadContextCollection& _thread_context_collection;
  HandlerCollection const& _handler_collection;
  std::unique_ptr<RdtscClock> _rdtsc_clock{nullptr}; /** rdtsc clock if enabled **/

  std::thread _backend_worker_thread; /** the backend thread that is writing the log to the handlers */
  std::chrono::nanoseconds _backend_thread_sleep_duration; /** backend_thread_sleep_duration from config **/
  std::once_flag _start_init_once_flag; /** flag to start the thread only once, in case start() is called multiple times */
  bool _has_unflushed_messages{false}; /** There are messages that are buffered by the OS, but not yet flushed */
  std::atomic<bool> _is_running{false}; /** The spawned backend thread status */
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
    // Set the backend worker thread status
    _is_running.store(true, std::memory_order_relaxed);

    // We store the configuration here on our local variable since the config flag is not atomic
    // and we don't want it to change after we have started - This is just for safety and to
    // enforce the user to configure a variable before the thread has started
    _backend_thread_sleep_duration = _config.backend_thread_sleep_duration();

    std::thread worker([this]() {
      // On Start
      if (_config.backend_thread_cpu_affinity() != std::numeric_limits<uint16_t>::max())
      {
        // Set cpu affinity if requested to cpu _backend_thread_cpu_affinity
        set_cpu_affinity(_config.backend_thread_cpu_affinity());
      }

      // Set the thread name to the desired name
      set_thread_name(_config.backend_thread_name().data());

#if (QUILL_RDTSC_CLOCK == 1)
      // Use rdtsc clock based on config. The clock requires a few seconds to init as it is
      // taking samples first
      _rdtsc_clock = std::make_unique<RdtscClock>(std::chrono::milliseconds{QUILL_RDTSC_RESYNC_INTERVAL});
#endif

      // Running
      while (QUILL_LIKELY(is_running()))
      {
        _main_loop();
      }

      // On exit
      _exit();
    });

    // Move the worker ownership to our class
    _backend_worker_thread.swap(worker);
  });
}

/***/
bool BackendWorker::_process_record(ThreadContextCollection::backend_thread_contexts_cache_t const& thread_contexts)
{
  // Iterate through all records in all thread contexts queues and find the one with the lowest
  // rdtsc to process We will log the timestamps in order
  uint64_t min_rdtsc = std::numeric_limits<uint64_t>::max();
  ThreadContext::SPSCQueueT::handle_t desired_record_handle;
  char const* desired_thread_id{nullptr};

  for (auto& elem : thread_contexts)
  {
    // search all queues and get the first record from each queue if there is any
    auto observed_record_handle = elem->spsc_queue().try_pop();

    if (observed_record_handle.is_valid())
    {
      if (observed_record_handle.data()->timestamp() < min_rdtsc)
      {
        // we found a new min rdtsc
        min_rdtsc = observed_record_handle.data()->timestamp();

        // if we were holding previously a RecordBase handle we need to release it, otherwise it will
        // get destructed and we will remove the record from the queue that we don't want
        // we release to only observe and not remove the Record from the queue
        desired_record_handle.release();

        // Move the current record handle to maintain it's lifetime
        desired_record_handle = std::move(observed_record_handle);

        // Also store the caller thread id of this log record
        desired_thread_id = elem->thread_id();
      }
      else
      {
        // we found a record with a greater rdtsc value than our current and we are not interested
        // we release to only observe and not remove the record from the queue
        observed_record_handle.release();
      }
    }
  }

  if (QUILL_UNLIKELY(!desired_record_handle.is_valid()))
  {
    // there is nothing to process and we have nothing to do
    return false;
  }

  // A lambda to obtain the logger details and pass them to RecordBase, this lambda is called only
  // in case we need to flush because we are processing a CommandRecord
  auto obtain_active_handlers = [this]() { return _handler_collection.active_handlers(); };

  // Get the log record timestamp and convert it to a real timestamp in nanoseconds from epoch
  std::chrono::nanoseconds const log_record_ts = _get_real_timestamp(desired_record_handle);

  desired_record_handle.data()->backend_process(desired_thread_id, obtain_active_handlers, log_record_ts);

  return true;
}

/***/
void BackendWorker::_main_loop()
{
  // load all contexts locally in case any new ThreadContext (new thread) was added
  ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  bool const processed_record = _process_record(cached_thread_contexts);

  if (processed_record)
  {
    // we have found and processed a log record

    // Since after processing a log record we never force flush and leave it up to the OS instead
    // keep track of how unflushed messages we have
    _has_unflushed_messages = true;
  }
  else
  {
    // None of the thread local queues had any log record to process, this means we have processed
    // all messages in all queues We will force flush any unflushed messages and then sleep
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

    // Sleep for the specified duration as we found no records in any of the queues to process
    std::this_thread::sleep_for(_backend_thread_sleep_duration);
  }
}

/***/
std::chrono::nanoseconds BackendWorker::_get_real_timestamp(ThreadContext::SPSCQueueT::handle_t const& log_record_handle) const noexcept
{
#if (QUILL_RDTSC_CLOCK == 1)

  assert(_rdtsc_clock && "rdtsc should not be nullptr");
  assert(log_record_handle.data()->using_rdtsc() &&
         "RecordBase has a std::chrono timestamp, but the backend thread is using rdtsc timestamp");

  // pass to our clock the stored rdtsc from the caller thread
  return _rdtsc_clock->time_since_epoch(log_record_handle.data()->timestamp());
#else
  assert(!_rdtsc_clock && "rdtsc should be nullptr");
  assert(!log_record_handle.data()->using_rdtsc() &&
         "RecordBase has a rdtsc clock timestamp, but the backend thread is using std::chrono "
         "timestamp");

  // Then the timestamp() will be already in epoch no need to convert it like above
  // The precision of system_clock::time-point is not portable across platforms.
  std::chrono::system_clock::duration const timestamp_duration{log_record_handle.data()->timestamp()};
  return std::chrono::nanoseconds{timestamp_duration};
#endif
}

} // namespace detail
} // namespace quill