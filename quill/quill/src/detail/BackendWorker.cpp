#include "quill/detail/BackendWorker.h"

#include <vector>

#include "quill/detail/ThreadContext.h"
#include "quill/detail/misc/Os.h"

namespace quill
{
namespace detail
{
/***/
BackendWorker::BackendWorker(Config const& config,
                             ThreadContextCollection& thread_context_collection,
                             HandlerCollection const& handler_collection)
  : _config(config), _thread_context_collection(thread_context_collection), _handler_collection(handler_collection)
{
}

/***/
BackendWorker::~BackendWorker()
{
  // This destructor will run during static destruction as the thread is part of the singleton
  stop();
}

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

#if defined(QUILL_RDTSC_CLOCK)
      // Use rdtsc clock based on config. The clock requires a few seconds to init as it is
      // taking samples first
      _rdtsc_clock = std::make_unique<RdtscClock>();
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
void BackendWorker::stop() noexcept
{
  // Stop the backend worker
  _is_running.store(false, std::memory_order_relaxed);

  // Wait the backend thread to join, if backend thread was never started it won't be joinable so we can still
  if (_backend_worker_thread.joinable())
  {
    _backend_worker_thread.join();
  }
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
void BackendWorker::_exit()
{
  // load all contexts locally
  ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  while (_process_record(cached_thread_contexts))
  {
    // loop until there are no log records left
  }
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

  desired_record_handle.data()->backend_process(desired_thread_id, obtain_active_handlers,
                                                _rdtsc_clock.get());

  return true;
}

} // namespace detail
} // namespace quill