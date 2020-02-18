#include "quill/detail/LogManager.h"

#include "quill/detail/misc/Spinlock.h"
#include "quill/detail/record/CommandRecord.h"

namespace quill
{
namespace detail
{

/***/
void LogManager::flush()
{
  if (!_backend_worker.is_running())
  {
    // Backend worker needs to be running, otherwise we are stuck for ever waiting
    return;
  }

  // Create an atomic variable
  std::atomic<bool> backend_thread_flushed{false};

  // notify will be invoked done the backend thread when this message is processed
  auto notify_callback = [&backend_thread_flushed]() {
    // When the backend thread is done flushing it will set the flag to true
    backend_thread_flushed.store(true);
  };

  using log_record_t = detail::CommandRecord;
  bool pushed;
  do
  {
    pushed = _thread_context_collection.local_thread_context()->spsc_queue().try_emplace<log_record_t>(notify_callback);
    // unlikely case if the queue gets full we will wait until we can log
  } while (QUILL_UNLIKELY(!pushed));

  // The caller thread keeps checking the flag until the backend thread flushes
  do
  {
    // wait
    std::this_thread::sleep_for(std::chrono::nanoseconds{100});
  } while (!backend_thread_flushed.load());
}

/***/
void LogManager::start_backend_worker() { _backend_worker.run(); }

/***/
void LogManager::stop_backend_worker() { _backend_worker.stop(); }

} // namespace detail
} // namespace quill