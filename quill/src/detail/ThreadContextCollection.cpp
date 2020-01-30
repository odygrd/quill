#include "quill/detail/ThreadContextCollection.h"

namespace quill::detail
{
/***/
ThreadContextCollection::ThreadContextWrapper::ThreadContextWrapper(ThreadContextCollection const& thread_context_collection)
  : _thread_context(std::make_shared<ThreadContext>())
{
  thread_context_collection.register_thread_context(_thread_context);
}

/***/
ThreadContextCollection::ThreadContextWrapper::~ThreadContextWrapper() noexcept
{
  // This destructor will get called when the thread that created this wrapper stops
  // we will only invalidate the thread context
  // The backend thread will empty an invalidated ThreadContext and then remove it from
  // the ThreadContextCollection
  // There is only exception for the thread who owns the ThreadContextCollection the
  // main thread. The thread context of the main thread can get deleted before getting invalidated
  _thread_context->invalidate();
}

/***/
void ThreadContextCollection::register_thread_context(std::shared_ptr<ThreadContext> const& thread_context) const
{
  std::lock_guard<Spinlock> const lock(_spinlock);
  _thread_contexts.push_back(thread_context);
  _set_changed();
}

/***/
std::vector<ThreadContext*> const& ThreadContextCollection::backend_thread_contexts_cache()
{
  // Remove any invalidated contexts
  _find_and_remove_invalidated_thread_contexts();

  // Check if _thread_contexts has changed. This can happen only when a new thread context is added by any Logger
  if (_has_changed())
  {
    // if the thread _thread_contexts was changed we lock and remake our reference cache
    std::lock_guard<Spinlock> const lock(_spinlock);
    _thread_context_cache.clear();

    // Remake thread context ref
    for (auto const& elem : _thread_contexts)
    {
      // We do skip invalidated && empty queue thread contexts as this is very rare, so instead
      // we just add them and expect them to be cleaned in the next iteration
      _thread_context_cache.push_back(elem.get());
    }
  }

  return _thread_context_cache;
}

/***/
bool ThreadContextCollection::_has_changed() const noexcept
{
  // Again relaxed memory model as in case it is false we will acquire the lock
  if (_changed.load(std::memory_order_relaxed))
  {
    // if the variable was updated to true, set it to false,
    // There should not be any race condition here as this is the only place _changed is set to false
    _changed.store(false, std::memory_order_relaxed);
    return true;
  }
  return false;
}

/***/
void ThreadContextCollection::_set_changed() const noexcept
{
  // Set changed is used with the lock, we can have relaxed memory order here as the lock
  // is acq/rel anyway
  return _changed.store(true, std::memory_order_relaxed);
}

/***/
void ThreadContextCollection::_remove_shared_invalidated_thread_context(ThreadContext const* thread_context)
{
  std::lock_guard<Spinlock> const lock(_spinlock);

  auto thread_context_it = std::find_if(_thread_contexts.begin(), _thread_contexts.end(),
                                        [thread_context](std::shared_ptr<ThreadContext> const& elem) {
                                          return elem.get() == thread_context;
                                        });

  assert(thread_context_it != _thread_contexts.end() &&
         "Attempting to remove a non existent thread context");

  assert(!thread_context_it->get()->is_valid() && "Attempting to remove a valid thread context");

  assert(thread_context_it->get()->spsc_queue().empty() &&
         "Attempting to remove a thread context with a non empty queue");

  _thread_contexts.erase(thread_context_it);

  // we don't set changed here as this is called only by the backend thread and it updates
  // the thread_contexts_cache itself after this function
}

/***/
void ThreadContextCollection::_find_and_remove_invalidated_thread_contexts()
{
  // First we iterate our existing cache and we look for any invalidated contexts
  auto found_invalid_and_empty_thread_context = std::find_if(
    _thread_context_cache.cbegin(), _thread_context_cache.cend(), [](ThreadContext const* thread_context) {
      // If the thread context is invalid it means the thread that created it has now died.
      // We also want to empty the queue from all LogRecords before removing the thread context
      return !thread_context->is_valid() && thread_context->spsc_queue().empty();
    });

  while (QUILL_UNLIKELY(found_invalid_and_empty_thread_context != _thread_context_cache.cend()))
  {
    // if we found anything then remove it - Here if we have more than one to remove we will try to
    // acquire the lock multiple times but it should be fine as it is unlikely to have that many
    // to remove
    _remove_shared_invalidated_thread_context(*found_invalid_and_empty_thread_context);

    // We also need to remove this from our local _thread_context_cache
    _thread_context_cache.erase(found_invalid_and_empty_thread_context);

    // And then look again
    found_invalid_and_empty_thread_context = std::find_if(
      _thread_context_cache.cbegin(), _thread_context_cache.cend(), [](ThreadContext const* thread_context) {
        // If the thread context is invalid it means the thread that created it has now died.
        // We also want to empty the queue from all LogRecords before removing the thread context
        return !thread_context->is_valid() && thread_context->spsc_queue().empty();
      });
  }
}
} // namespace quill::detail