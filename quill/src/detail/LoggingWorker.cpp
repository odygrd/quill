#include "quill/detail/LoggingWorker.h"

#include <vector>

#include "quill/detail/Message.h"
#include "quill/detail/ThreadContext.h"
#include "quill/detail/ThreadContextCollection.h"

#include <iostream> // todo:: remove me
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
  stop();

  assert(_logging_thread.joinable() && "logging thread needs to be joinable");
  _logging_thread.join();
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
    });

    // Move the worker ownership to our class
    _logging_thread.swap(worker);
  });
}

/***/
void LoggingWorker::stop() noexcept { _is_running.store(false, std::memory_order_relaxed); }

/***/
void LoggingWorker::_main_loop()
{
  // load all contexts locally in case any new ThreadContext (new thread) was added
  std::vector<ThreadContext*> const& cached_thead_contexts =
    _thread_context_collection.get_cached_thread_contexts();

  _check_for_messages(cached_thead_contexts);
}

/***/
void LoggingWorker::_check_for_messages(std::vector<ThreadContext*> const& thread_contexts)
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

        if (min_rdtsc_message_handle.is_valid())
        {
          // if we were holding a valid message handle we need to release it, otherwise it will
          // get destructed and we will lose the message
          // we release to only observe and not remove the Message from the queue
          min_rdtsc_message_handle.release();
        }

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
    return;
  }

  // TODO:: add sink collection class and pass it to process
  min_rdtsc_message_handle.data()->process(min_rdtsc_thread_id);
}

} // namespace quill::detail