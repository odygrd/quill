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
  if (!_error_handler)
  {
    // set up the default error handler
    _error_handler = [](std::string const& s) { std::cerr << s << std::endl; };
  }
}

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


} // namespace detail
} // namespace quill