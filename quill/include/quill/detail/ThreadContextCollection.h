/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Config.h"
#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_HOT
#include "quill/detail/misc/Common.h"     // for CACHE_LINE_ALIGNED
#include <atomic>                         // for atomic
#include <cassert>                        // for assert
#include <cstdint>                        // for uint8_t
#include <memory>                         // for shared_ptr
#include <mutex>
#include <vector> // for vector

namespace quill
{
/** forward declarations **/
struct Config;

namespace detail
{

class ThreadContext;

/**
 * ThreadContextCollection class
 * a) Creates or returns the existing thread local ThreadContext instance to the thread that called Logger.log()
 * b) Provides the backend thread with a cache which contains all active ThreaadContexts so that the backend thread
 * can iterate through them and process the messages in each queue
 */
class ThreadContextCollection
{
private:
  /**
   * Class used to wrap a ThreadContext. Used as a thread local object.
   * The constructor always registers a new context to the context collection and
   * the destructor always invalidates it.
   * The backend thread will later clean invalidated contexts when there is nothing else to log
   */
  template <QueueType queue_type>
  class ThreadContextWrapper
  {
  public:
    /**
     * Constructor
     *
     * Called by each caller thread once
     * Creates a new context and then registers it to the context collection sharing ownership
     * of the ThreadContext
     */
    ThreadContextWrapper(ThreadContextCollection& thread_context_collection, uint32_t default_queue_capacity,
                         uint32_t initial_transit_event_buffer_capacity, bool huge_pages)
      : _thread_context_collection(thread_context_collection),
        _thread_context(std::shared_ptr<ThreadContext>(new ThreadContext(
          queue_type, default_queue_capacity, initial_transit_event_buffer_capacity, huge_pages)))
    {
      // We can not use std::make_shared above.
      // Explanation :
      // ThreadContext has the SPSC queue as a class member which requires a 64 cache byte alignment,
      // since we are creating this object on the heap this is not guaranteed.
      // Visual Studio is the only compiler that gives a warning that ThreadContext might not be aligned to 64 bytes,
      // as it is allocated on the heap, and we should use aligned alloc instead.
      // The solution to solve this would be to define a custom operator new and operator delete for ThreadContext.
      // However, when using std::make_shared, the default allocator is used.
      // This is a problem if the class is supposed to use a non-default allocator like ThreadContext
      _thread_context_collection.register_thread_context(_thread_context);
    }

    /**
     * Deleted
     */
    ThreadContextWrapper(ThreadContextWrapper const&) = delete;
    ThreadContextWrapper& operator=(ThreadContextWrapper const&) = delete;

    /**
     * Destructor
     * Invalidates the thread context on the thread destruction but the ThreadContext is not
     * destroyed yet as ownership is shared
     */
    ~ThreadContextWrapper() noexcept
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

    /**
     * @return The pointer to this thread context
     */
    QUILL_NODISCARD_ALWAYS_INLINE_HOT ThreadContext* thread_context() const noexcept
    {
      assert(_thread_context && "_thread_context can not be null");
      return _thread_context.get();
    }

  private:
    ThreadContextCollection& _thread_context_collection;
    /**<
     * This could be unique_ptr but the thread context of main thread that owns
     * ThreadContextCollection can be destructed last even after the logger singleton destruction
     * so we use shared_ptr */
    std::shared_ptr<ThreadContext> _thread_context;
  };

public:
  /**
   * Type definitions
   */
  using backend_thread_contexts_cache_t = std::vector<ThreadContext*>;

public:
  /**
   * Constructor
   */
  explicit ThreadContextCollection(Config const& config) : _config(config) {}

  /**
   * Destructor
   */
  ~ThreadContextCollection() = default;

  /**
   * Deleted
   */
  ThreadContextCollection(ThreadContextCollection const&) = delete;
  ThreadContextCollection& operator=(ThreadContextCollection const&) = delete;

  /**
   * Creates a new thread context or returns the existing one that was created for that thread.
   * Called by caller threads
   * @return A reference to the specific for this thread thread context
   */
  template <QueueType queue_type>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT ThreadContext* local_thread_context() noexcept
  {
    static thread_local ThreadContextWrapper<queue_type> thread_context_wrapper{
      *this, _config.default_queue_capacity,
      _config.backend_thread_use_transit_buffer ? _config.backend_thread_initial_transit_event_buffer_capacity : 1,
      _config.enable_huge_pages_hot_path};
    return thread_context_wrapper.thread_context();
  }

  /**
   * Register a newly created thread context.
   * Called by caller threads
   * @param thread_context thread context to register
   */
  void register_thread_context(std::shared_ptr<ThreadContext> const& thread_context)
  {
    _mutex.lock();
    _thread_contexts.push_back(thread_context);
    _mutex.unlock();
    _set_new_thread_context();
  }

  /**
   * Reloads the thread contexts in our local cache.
   * Any invalidated thread contexts with empty queues are removed and any new thread contexts
   * from new threads are added to the returned vector of caches.
   * If there are no invalidated contexts or no new contexts the existing cache is returned
   * @return All current owned thread contexts
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT backend_thread_contexts_cache_t const& backend_thread_contexts_cache()
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

  /**
   * Clears thread context cache from invalid and empty thread contexts
   */
  QUILL_ATTRIBUTE_HOT void clear_invalid_and_empty_thread_contexts()
  {
    if (QUILL_UNLIKELY(_has_invalid_thread_context()))
    {
      // Remove any invalidated contexts, this can happen only when a thread is terminating
      _find_and_remove_invalidated_thread_contexts();
    }
  }

private:
  /**
   * Return true if _thread_contexts have changed
   * @note Only accessed by the backend thread
   * @return true if the shared data structure was changed by any calls to Logger
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool _has_new_thread_context() noexcept
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

  /**
   * Indicate that the context has changed. A new thread context has been added or removed
   * @note Only called by the caller threads
   */
  void _set_new_thread_context() noexcept
  {
    // Set changed is used with the lock, we can have relaxed memory order here as the lock
    // is acq/rel anyway
    return _new_thread_context.store(true, std::memory_order_relaxed);
  }

  /**
   * Increment the counter for a removed thread context. This notifies the backend thread to look for an invalidated context
   */
  void _add_invalid_thread_context() noexcept
  {
    // relaxed is fine, see _has_invalid_thread_context explanation
    _invalid_thread_context.fetch_add(1, std::memory_order_relaxed);
  }

  /**
   * Reduce the value of thread context removed counter. This is decreased by the backend thread
   * when we found and removed the invalided context
   */
  void _sub_invalid_thread_context() noexcept
  {
    // relaxed is fine, see _has_invalid_thread_context explanation
    _invalid_thread_context.fetch_sub(1, std::memory_order_relaxed);
  }

  /**
   * @return True if there is an invalid thread context
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool _has_invalid_thread_context() const noexcept
  {
    // Here we do relaxed because if the value is not zero we will look inside ThreadContext invalid
    // flag that is also a relaxed atomic, and then we will look into the SPSC queue size that is
    // also atomic Even if we don't read everything in order we will check again in the next circle
    return _invalid_thread_context.load(std::memory_order_relaxed) != 0;
  }

  /**
   * Remove a thread context from our main thread context collection
   * Removing aan invalidated context from the collection will result in ThreadContext as neither
   * the thread or this class will hold the shared pointer anymore
   * will be using the shared pointer anymore
   *
   * @note Only called by the backend thread
   * @param thread_context the pointer to the thread context we are removing
   */
  void _remove_shared_invalidated_thread_context(ThreadContext const* thread_context)
  {
    std::lock_guard<std::mutex> const lock(_mutex);

    auto thread_context_it = std::find_if(_thread_contexts.begin(), _thread_contexts.end(),
                                          [thread_context](std::shared_ptr<ThreadContext> const& elem)
                                          { return elem.get() == thread_context; });

    assert(thread_context_it != _thread_contexts.end() &&
           "Attempting to remove_file a non existent thread context");

    assert(!thread_context_it->get()->is_valid() &&
           "Attempting to remove_file a valid thread context");

    assert(thread_context_it->get()->spsc_queue<QUILL_QUEUE_TYPE>().empty() &&
           "Attempting to remove_file a thread context with a non empty queue");

    _thread_contexts.erase(thread_context_it);

    // we don't set changed here as this is called only by the backend thread and it updates
    // the thread_contexts_cache itself after this function
  }

  /**
   * Looks into the _thread_context_cache and removes all thread contexts that are 1) invalidated
   * and 2) have an empty queue of no events to process
   *
   * @note Only called by the backend thread
   */
  void _find_and_remove_invalidated_thread_contexts()
  {
    // First we iterate our existing cache and we look for any invalidated contexts
    auto found_invalid_and_empty_thread_context = std::find_if(
      _thread_context_cache.begin(), _thread_context_cache.end(),
      [](ThreadContext* thread_context)
      {
        // If the thread context is invalid it means the thread that created it has now died.
        // We also want to empty the queue from all LogRecords before removing the thread context

        return !thread_context->is_valid() && thread_context->spsc_queue<QUILL_QUEUE_TYPE>().empty() &&
          thread_context->transit_event_buffer().empty();
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
        _thread_context_cache.begin(), _thread_context_cache.end(),
        [](ThreadContext* thread_context)
        {
          // If the thread context is invalid it means the thread that created it has now died.
          // We also want to empty the queue from all LogRecords before removing the thread context

          return !thread_context->is_valid() && thread_context->spsc_queue<QUILL_QUEUE_TYPE>().empty();
        });
    }
  }

private:
  Config const& _config;
  std::mutex _mutex; /**< Protect access when register contexts or removing contexts */
  std::vector<std::shared_ptr<ThreadContext>> _thread_contexts; /**< The registered contexts */

  /**<
   * A reference to the owned thread contexts that we update when there is any change. We do
   * this so the backend thread does not hold the mutex lock all the time while it is trying to
   * process events as it is iterating through the thread contexts all the time
   *
   * @note Accessed only by the backend thread
   * */
  backend_thread_contexts_cache_t _thread_context_cache;

  /**< Indicator that a new context was added, set by caller thread to true, read by the backend thread only, updated by any thread */
  alignas(CACHE_LINE_ALIGNED) std::atomic<bool> _new_thread_context{false};

  /**<
   * Indicator of how many thread contexts are removed, if this number is not zero we will search for invalidated and empty
   * queue context until we find it to remove_file it.
   * Incremented by any thread on thread local destruction, decremented by the backend thread
   */
  alignas(CACHE_LINE_ALIGNED) std::atomic<uint8_t> _invalid_thread_context{0};
};
} // namespace detail
} // namespace quill
