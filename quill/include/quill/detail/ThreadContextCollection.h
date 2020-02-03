#pragma once

#include "quill/detail/ThreadContext.h"
#include "quill/detail/utility/Spinlock.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

namespace quill::detail
{
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
    explicit ThreadContextWrapper(ThreadContextCollection& thread_context_collection);

    /**
     * Destructor
     * Invalidates the thread context on the thread destruction but the ThreadContext is not
     * destroyed yet as ownership is shared
     */
    ~ThreadContextWrapper() noexcept;

    /**
     * @return The pointer to this thread context
     */
    inline ThreadContext* thread_context() const noexcept
    {
      assert(_thread_context && "_thread_context can not be null");
      return _thread_context.get();
    }

  private:
    /**<
     * This could be unique_ptr but the thread context of main thread that owns
     * ThreadContextCollection can be destructed last even after the logger singleton destruction
     * so we use shared_ptr */
    std::shared_ptr<ThreadContext> _thread_context;
  };

public:
  /**
   * Constructor
   */
  ThreadContextCollection() = default;

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
  inline ThreadContext* local_thread_context() noexcept
  {
    static thread_local ThreadContextWrapper thread_context_wrapper{*this};
    return thread_context_wrapper.thread_context();
  }

  /**
   * Register a newly created thread context.
   * Called by caller threads
   * @param thread_context
   */
  void register_thread_context(std::shared_ptr<ThreadContext> const& thread_context);

  /**
   * Reloads the thread contexts in our local cache.
   * Any invalidated thread contexts with empty queues are removed and any new thread contexts
   * from new threads are added to the returned vector of caches.
   * If there are no invalidated contexts or no new contexts the existing cache is returned
   * @return All current owned thread contexts
   */
  std::vector<ThreadContext*> const& backend_thread_contexts_cache();

private:
  /**
   * Return true if _thread_contexts have changed
   * @note Only accessed by the backend thread
   * @return true if the shared data structure was changed by any calls to Logger
   */
  [[nodiscard]] bool _has_changed() noexcept;

  /**
   * Indicate that the context has changed. A new thread context has been added or removed
   * @note Only called by the caller threads
   */
  void _set_changed() noexcept;

  /**
   * Remove a thread context from our main thread context collection
   * Removing aan invalidated context from the collection will result in ThreadContext as neither
   * the thread or this class will hold the shared pointer anymore
   * will be using the shared pointer anymore
   *
   * @note Only called by the backend thread
   * @param thread_context the pointer to the thread context we are removing
   */
  void _remove_shared_invalidated_thread_context(ThreadContext const* thread_context);

  /**
   * Looks into the _thread_context_cache and removes all thread contexts that are invalidated
   * and have an empty queue of LogRecords
   *
   * @note Only called by the backend thread
   */
  void _find_and_remove_invalidated_thread_contexts();

private:
  Spinlock _spinlock; /**< Protect access when register contexts or removing contexts */
  std::vector<std::shared_ptr<ThreadContext>> _thread_contexts; /**< The registered contexts */

  /**<
   * A reference to the owned thread contexts that we update when there is any change. We do
   * this so the backend thread does not hold the mutex lock all the time while it is trying to
   * process LogRecords as it is iterating through the thread contexts all the time
   *
   * @note Accessed only by the backend thread
   * */
  std::vector<ThreadContext*> _thread_context_cache;

  /**< Indicator that a new context was added or removed, set by caller thread to true, read by the backend thread only */
  std::atomic<bool> _changed{false};
};

} // namespace quill::detail