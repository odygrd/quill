#include "quill/detail/ThreadContextCollection.h"

namespace quill::detail
{
/***/
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT ThreadContextCollection::backend_thread_contexts_cache_t const& ThreadContextCollection::backend_thread_contexts_cache()
{
  // Check if _thread_contexts has changed. This can happen only when a new thread context is added by any Logger
  if (QUILL_UNLIKELY(_has_new_thread_context()))
  {
    // if the thread _thread_contexts was changed we lock and remake our reference cache
    std::lock_guard<std::mutex> const lock{_mutex};
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
bool ThreadContextCollection::_has_new_thread_context() noexcept
{
  // Again relaxed memory model as in case it is false we will acquire the lock
  if (_new_thread_context.load(std::memory_order_relaxed))
  {
    // if the variable was updated to true, set it to false,
    // There should not be any race condition here as this is the only place _changed is set to
    // false, and we will return true anyway
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
  // flag that is also a relaxed atomic, and then we will look into the SPSC queue size that is
  // also atomic Even if we don't read everything in order we will check again in the next circle
  return _invalid_thread_context.load(std::memory_order_relaxed) != 0;
}
} // namespace quill::detail