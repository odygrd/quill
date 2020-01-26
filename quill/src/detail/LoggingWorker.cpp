#include "quill/detail/LoggingWorker.h"

#include <vector>

#include "quill/LogMacros.h" // for config definitions
#include "quill/detail/ThreadContext.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/message/MessageBase.h"

namespace quill::detail
{
/***/
LoggingWorker::LoggingWorker(ThreadContextCollection& thread_context_collection)
  : _thread_context_collection(thread_context_collection)
{
}

/***/
LoggingWorker::~LoggingWorker()
{
  // This destructor will run during static destruction as the thread is part of the singleton
  stop();
}

/***/
bool LoggingWorker::is_running() const noexcept
{
  return _is_running.load(std::memory_order_relaxed);
}

/***/
void LoggingWorker::run()
{
  // protect init to be called only once
  std::call_once(_start_init_once_flag, [this]() {
    // Set the logging worker status
    _is_running.store(true, std::memory_order_relaxed);

    std::thread worker([this]() {
      while (is_running())
      {
        _main_loop();
      }

      // on exit
      _exit();
    });

    // Move the worker ownership to our class
    _logging_thread.swap(worker);
  });
}

/***/
void LoggingWorker::stop() noexcept
{
  // Stop the logging worker
  _is_running.store(false, std::memory_order_relaxed);

  // Wait the logging worker it to join
  if (_logging_thread.joinable())
  {
    // if logging worker thread was never started it won't be joinable
    _logging_thread.join();
  }
}

/***/
void LoggingWorker::_main_loop()
{
  // load all contexts locally in case any new ThreadContext (new thread) was added
  std::vector<ThreadContext*> const& cached_thead_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  bool const had_log_record = _check_for_messages(cached_thead_contexts);

  if (!had_log_record)
  {
    // Sleep for the specified duration as we found no messages
#if QUILL_BACKEND_THREAD_SLEEP_DURATION_NS > 0
    std::this_thread::sleep_for(std::chrono::nanoseconds{QUILL_BACKEND_THREAD_SLEEP_DURATION_NS});
#endif
  }
}

/***/
void LoggingWorker::_exit()
{
  // load all contexts locally
  std::vector<ThreadContext*> const& cached_thead_contexts =
    _thread_context_collection.backend_thread_contexts_cache();

  while (_check_for_messages(cached_thead_contexts))
  {
    // loop until there are no log records left
  }
}

/***/
bool LoggingWorker::_check_for_messages(std::vector<ThreadContext*> const& thread_contexts)
{
  // We will log timestamps in order
  // Iterate through all messages in all thread contexts queues and find the one with the lowest rdtsc to process
  uint64_t min_rdtsc = std::numeric_limits<uint64_t>::max();
  ThreadContext::SPSCQueueT::Handle min_rdtsc_message_handle;
  uint32_t min_rdtsc_thread_id = 0;

  for (auto& elem : thread_contexts)
  {
    // search all queues and get the first Message from ech queue if there is any
    auto message_handle = elem->spsc_queue().try_pop();

    if (message_handle.is_valid())
    {
      if (message_handle.data()->rdtsc() < min_rdtsc)
      {
        // we found a new min rdtsc
        min_rdtsc = message_handle.data()->rdtsc();

        // if we were holding previously aa message handle we need to release it, otherwise it will
        // get destructed and we will lose the message
        // we release to only observe and not remove the Message from the queue
        min_rdtsc_message_handle.release();

        // Move the current message handle to maintain it's lifetime
        min_rdtsc_message_handle = std::move(message_handle);

        // Also store the caller thread id of this message
        min_rdtsc_thread_id = elem->thread_id();
      }
      else
      {
        // we found a message with a greater rdtsc value and we are not interested
        // we release to only observe and not remove the Message from the queue
        message_handle.release();
      }
    }
  }

  if (!min_rdtsc_message_handle.is_valid())
  {
    // there are no messages to log
    return false;
  }

  // TODO:: add sink collection class and pass it to process
  min_rdtsc_message_handle.data()->backend_process(min_rdtsc_thread_id);

  return true;
}

} // namespace quill::detail