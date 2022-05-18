#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/ThreadContext.h" // for ThreadContext, ThreadContext...
#include "quill/detail/misc/Common.h"   // for QUILL_UNLIKELY
#include <algorithm>                    // for find_if
#include <mutex>                        // for lock_guard
#include <type_traits>                  // for remove_extent<>::type

namespace quill
{
namespace detail
{
/***/
ThreadContextCollection::ThreadContextWrapper::ThreadContextWrapper(ThreadContextCollection& thread_context_collection)
  : _thread_context_collection(thread_context_collection),
    _thread_context(std::shared_ptr<ThreadContext>(new ThreadContext()))
{
  // We can not use std::make_shared above.
  // Explanation :
  // ThreadContext has the SPSC queue as a class member which requires a 64 cache byte alignment,
  // since we are creating this object on the heap this is not guaranteed.
  // Visual Studio is the only compiler that gives a warning that ThreadContext might not be aligned to 64 bytes,
  // as it is allocated on the heap and we should use aligned alloc instead.
  // The solution to solve this would be to define a custom operator new and operator delete for ThreadContext.
  // However, when using std::make_shared, the default allocator is used.
  // This is a problem if the class is supposed to use a non-default allocator like ThreadContext
  _thread_context_collection.register_thread_context(_thread_context);
}

/***/
ThreadContextCollection::ThreadContextWrapper::~ThreadContextWrapper() noexcept
{
  // This destructor will get called when the thread that created this wrapper stops
  // we will only invalidate the thread context
  // The backend thread will empty an invalidated ThreadContext and then remove_file it from
  // the ThreadContextCollection
  // There is only exception for the thread who owns the ThreadContextCollection the
  // main thread. The thread context of the main thread can get deleted before getting invalidated
  _thread_context->invalidate();

  // Notify the backend thread that one context has been removed
  _thread_context_collection._add_invalid_thread_context();
}

/***/
void ThreadContextCollection::register_thread_context(std::shared_ptr<ThreadContext> const& thread_context)
{
  _mutex.lock();
  _thread_contexts.push_back(thread_context);
  _mutex.unlock();
  _set_new_thread_context();
}

/***/
ThreadContextCollection::backend_thread_contexts_cache_t const& ThreadContextCollection::backend_thread_contexts_cache()
{
  // Check if _thread_contexts has changed. This can happen only when a new thread context is added by any Logger
  if (QUILL_UNLIKELY(_has_new_thread_context()))
  {
    // if the thread _thread_contexts was changed we lock and remake our reference cache
    std::lock_guard<std::mutex> const lock(_mutex);
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
void ThreadContextCollection::clear_invalid_and_empty_thread_contexts()
{
  if (QUILL_UNLIKELY(_has_invalid_thread_context()))
  {
    // Remove any invalidated contexts, this can happen only when a thread is terminating
    _find_and_remove_invalidated_thread_contexts();
  }
}

/***/
bool ThreadContextCollection::_has_new_thread_context() noexcept
{
  // Again relaxed memory model as in case it is false we will acquire the lock
  if (_new_thread_context.load(std::memory_order_relaxed))
  {
    // if the variable was updated to true, set it to false,
    // There should not be any race condition here as this is the only place _changed is set to
    // false and we will return true anyway
    _new_thread_context.store(false, std::memory_order_relaxed);
    return true;
  }
  return false;
}

/***/
void ThreadContextCollection::_set_new_thread_context() noexcept
{
  // Set changed is used with the lock, we can have relaxed memory order here as the lock
  // is acq/rel anyway
  return _new_thread_context.store(true, std::memory_order_relaxed);
}

/***/
void ThreadContextCollection::_add_invalid_thread_context() noexcept
{
  // relaxed is fine, see _has_invalid_thread_context explanation
  _invalid_thread_context.fetch_add(1, std::memory_order_relaxed);
}

/***/
void ThreadContextCollection::_sub_invalid_thread_context() noexcept
{
  // relaxed is fine, see _has_invalid_thread_context explanation
  _invalid_thread_context.fetch_sub(1, std::memory_order_relaxed);
}

/***/
bool ThreadContextCollection::_has_invalid_thread_context() const noexcept
{
  // Here we do relaxed because if the value is not zero we will look inside ThreadContext invalid
  // flag that is also a relaxed atomic and then we will look into the SPSC queue size that is also
  // atomic Even if we don't read everything in order we will check again in the next circle
  return _invalid_thread_context.load(std::memory_order_relaxed) != 0;
}

/***/
void ThreadContextCollection::_remove_shared_invalidated_thread_context(ThreadContext const* thread_context)
{
  std::lock_guard<std::mutex> const lock(_mutex);

  auto thread_context_it = std::find_if(_thread_contexts.begin(), _thread_contexts.end(),
                                        [thread_context](std::shared_ptr<ThreadContext> const& elem)
                                        { return elem.get() == thread_context; });

  assert(thread_context_it != _thread_contexts.end() &&
         "Attempting to remove_file a non existent thread context");

  assert(!thread_context_it->get()->is_valid() &&
         "Attempting to remove_file a valid thread context");

  assert(thread_context_it->get()->spsc_queue().empty() &&
         "Attempting to remove_file a thread context with a non empty queue");

  _thread_contexts.erase(thread_context_it);

  // we don't set changed here as this is called only by the backend thread and it updates
  // the thread_contexts_cache itself after this function
}

/***/
void ThreadContextCollection::_find_and_remove_invalidated_thread_contexts()
{
  // First we iterate our existing cache and we look for any invalidated contexts
  auto found_invalid_and_empty_thread_context = std::find_if(
    _thread_context_cache.begin(), _thread_context_cache.end(), [](ThreadContext* thread_context)
                 {
                   // If the thread context is invalid it means the thread that created it has now died.
                   // We also want to empty the queue from all LogRecords before removing the thread context

                   return !thread_context->is_valid() && thread_context->spsc_queue().empty();
                 });

  while (QUILL_UNLIKELY(found_invalid_and_empty_thread_context != _thread_context_cache.cend()))
  {
    // Decrement the counter since we found something to remove_file
    _sub_invalid_thread_context();

    // if we found anything then remove it - Here if we have more than one to remove_file we will
    // try to acquire the lock multiple times but it should be fine as it is unlikely to have that
    // many to remove_file
    _remove_shared_invalidated_thread_context(*found_invalid_and_empty_thread_context);

    // We also need to remove_file this from _thread_context_cache, that is used only by the backend
    _thread_context_cache.erase(found_invalid_and_empty_thread_context);

    // And then look again
    found_invalid_and_empty_thread_context = std::find_if(
      _thread_context_cache.begin(), _thread_context_cache.end(), [](ThreadContext* thread_context)
                   {
                     // If the thread context is invalid it means the thread that created it has now died.
                     // We also want to empty the queue from all LogRecords before removing the thread context

                     return !thread_context->is_valid() && thread_context->spsc_queue().empty();
                   });
  }
}
} // namespace detail
} // namespace quill