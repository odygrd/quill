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
  // The logging thread will empty an invalidated ThreadContext and then remove it from
  // the ThreadContextCollection
  // There is only exception for the thread who owns the ThreadContextCollection the
  // main thread. The thread context of the main thread can get deleted before getting invalidated
  _thread_context->invalidate();
}

/***/
ThreadContextCollection::~ThreadContextCollection()
{
  // if there are any contexts left, ensure there are invalided and empty
  for (auto* thread_context : get_cached_thread_contexts())
  {
    // This will never happen and is just an extra check
    assert(thread_context->is_valid() && thread_context->spsc_queue().empty() &&
           "Destructing ThreadContextCollection that still has valid thread contexts");
  }
}

/***/
void ThreadContextCollection::register_thread_context(std::shared_ptr<ThreadContext> const& thread_context) const
{
  std::scoped_lock<std::mutex> guard(_mutex);
  _thread_contexts.push_back(thread_context);
  _set_changed();
}

/***/
void ThreadContextCollection::remove_thread_context(ThreadContext* thread_context)
{
  std::scoped_lock<std::mutex> guard(_mutex);

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
  _set_changed();
}

/***/
std::vector<ThreadContext*> const& ThreadContextCollection::get_cached_thread_contexts()
{
  if (_has_changed())
  {
    std::scoped_lock<std::mutex> guard(_mutex);
    _thread_context_cache.clear();

    // Remake thread context ref
    for (auto& elem : _thread_contexts)
    {
      _thread_context_cache.push_back(elem.get());
    }
  }

  return _thread_context_cache;
}

/***/
bool ThreadContextCollection::_has_changed() const noexcept
{
  return _changed.exchange(false, std::memory_order_acq_rel);
}

/***/
void ThreadContextCollection::_set_changed() const noexcept
{
  return _changed.store(true, std::memory_order_release);
}
} // namespace quill::detail