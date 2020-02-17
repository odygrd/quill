#include "quill/detail/LogManager.h"

#include "quill/detail/misc/Spinlock.h"
#include "quill/detail/record/CommandRecord.h"
#include <condition_variable>

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

  Spinlock spinlock;
  std::condition_variable_any cond;
  bool done = false;

  // notify will be invoked by the backend thread when this message is processed
  auto notify_callback = [&spinlock, &cond, &done]() {
    {
      std::lock_guard<Spinlock> const lock{spinlock};
      done = true;
    }
    cond.notify_one();
  };

  std::unique_lock<Spinlock> lock(spinlock);

  using log_record_t = detail::CommandRecord;
  bool pushed;
  do
  {
    pushed = _thread_context_collection.local_thread_context()->spsc_queue().try_emplace<log_record_t>(notify_callback);
    // unlikely case if the queue gets full we will wait until we can log
  } while (QUILL_UNLIKELY(!pushed));

  // Wait until notify is called
  cond.wait(lock, [&] { return done; });
}

/***/
void LogManager::start_backend_worker() { _backend_worker.run(); }

/***/
void LogManager::stop_backend_worker() { _backend_worker.stop(); }

} // namespace detail
} // namespace quill