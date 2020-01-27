#include "quill/detail/BackendWorker.h"

#include <vector>

#include "quill/detail/ThreadContext.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/record/RecordBase.h"

namespace quill::detail
{
/***/
BackendWorker::BackendWorker(Config const& config, ThreadContextCollection& thread_context_collection)
  : _config(config), _thread_context_collection(thread_context_collection)
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
      while (is_running())
      {
        _main_loop();
      }

      // on exit
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
  std::vector<ThreadContext*> const& cached_thead_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  bool const processed_record = _process_record(cached_thead_contexts);

  if (QUILL_UNLIKELY(!processed_record))
  {
    // Sleep for the specified duration as we found no records in any of the queues to process
    std::this_thread::sleep_for(_backend_thread_sleep_duration);
  }
}

/***/
void BackendWorker::_exit()
{
  // load all contexts locally
  std::vector<ThreadContext*> const& cached_thead_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  while (_process_record(cached_thead_contexts))
  {
    // loop until there are no log records left
  }
}

/***/
bool BackendWorker::_process_record(std::vector<ThreadContext*> const& thread_contexts)
{
  // Iterate through all records in all thread contexts queues and find the one with the lowest
  // rdtsc to process We will log the timestamps in order
  uint64_t min_rdtsc = std::numeric_limits<uint64_t>::max();
  ThreadContext::SPSCQueueT::Handle desired_record_handle;
  uint32_t desired_thread_id = 0;

  for (auto& elem : thread_contexts)
  {
    // search all queues and get the first record from each queue if there is any
    auto observed_record_handle = elem->spsc_queue().try_pop();

    if (observed_record_handle.is_valid())
    {
      if (observed_record_handle.data()->rdtsc() < min_rdtsc)
      {
        // we found a new min rdtsc
        min_rdtsc = observed_record_handle.data()->rdtsc();

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

  if (!desired_record_handle.is_valid())
  {
    // there is nothing to process
    return false;
  }

  // TODO:: add sink collection class and pass it to process
  desired_record_handle.data()->backend_process(desired_thread_id);

  // TODO:: When to flush on the sinks ? Maybe only if user requested
  return true;
}

} // namespace quill::detail