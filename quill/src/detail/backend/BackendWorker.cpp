#include "quill/detail/backend/BackendWorker.h"
#include "quill/detail/misc/FileUtilities.h"
#include <iostream> // for endl, basic_ostream, cerr, ostream
#include <vector>   // for vector

namespace quill
{
namespace detail
{
/***/
BackendWorker::BackendWorker(Config const& config, ThreadContextCollection& thread_context_collection,
                             HandlerCollection const& handler_collection)
  : _config(config), _thread_context_collection(thread_context_collection), _handler_collection(handler_collection)
{
#if !defined(QUILL_NO_EXCEPTIONS)
  if (!_error_handler)
  {
    // set up the default error handler
    _error_handler = [](std::string const& s) { std::cerr << s << std::endl; };
  }
#endif
}

/***/
BackendWorker::~BackendWorker()
{
  // This destructor will run during static destruction as the thread is part of the singleton
  stop();
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
uint32_t BackendWorker::thread_id() const noexcept { return _backend_worker_thread_id; }

#if !defined(QUILL_NO_EXCEPTIONS)
/***/
void BackendWorker::set_error_handler(backend_worker_error_handler_t error_handler)
{
  if (is_running())
  {
    QUILL_THROW(
      QuillError{"The backend thread has already started. The error handler must be set before the "
                 "thread starts."});
  }

  _error_handler = std::move(error_handler);
}
#endif

/***/
void BackendWorker::_check_dropped_messages(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts) noexcept
{
  // silence warning when bounded queue not used
  (void)cached_thread_contexts;

#if defined(QUILL_USE_BOUNDED_QUEUE)
  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    size_t const dropped_messages_cnt = thread_context->get_and_reset_message_counter();

    if (QUILL_UNLIKELY(dropped_messages_cnt > 0))
    {
      char ts[24];
      time_t t = time(nullptr);
      struct tm p;
      quill::detail::localtime_rs(std::addressof(t), std::addressof(p));
      strftime(ts, 24, "%X", std::addressof(p));

      // Write to stderr that we dropped messages
      std::string const msg = fmt::format("~ {} localtime dropped {} log messages from thread {}\n",
                                          ts, dropped_messages_cnt, thread_context->thread_id());

      detail::file_utilities::fwrite_fully(msg.data(), sizeof(char), msg.size(), stderr);
    }
  }
#endif
}

} // namespace detail
} // namespace quill