/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/AlignedAllocator.h" // for CacheAlignedAllocator
#include "quill/detail/misc/Attributes.h"       // for QUILL_ATTRIBUTE_HOT
#include "quill/detail/misc/Common.h"           // for CACHELINE_SIZE
#include "quill/detail/misc/Spinlock.h"         // for Spinlock
#include <atomic>                               // for atomic
#include <cassert>                              // for assert
#include <cstdint>                              // for uint8_t
#include <memory>                               // for shared_ptr
#include <vector>                               // for vector

namespace quill
{
namespace detail
{

/** forward declarations **/
class Config;
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
     * Deleted
     */
    ThreadContextWrapper(ThreadContextWrapper const&) = delete;
    ThreadContextWrapper& operator=(ThreadContextWrapper const&) = delete;

    /**
     * Destructor
     * Invalidates the thread context on the thread destruction but the ThreadContext is not
     * destroyed yet as ownership is shared
     */
    ~ThreadContextWrapper() noexcept;

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
  using backend_thread_contexts_cache_t =
    std::vector<ThreadContext*, detail::CacheAlignedAllocator<ThreadContext*>>;

public:
  /**
   * Constructor
   */
  explicit ThreadContextCollection(Config const& config) : _config(config){};

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
  QUILL_NODISCARD_ALWAYS_INLINE_HOT ThreadContext* local_thread_context() noexcept
  {
    static thread_local ThreadContextWrapper thread_context_wrapper{*this};
    return thread_context_wrapper.thread_context();
  }

  /**
   * Register a newly created thread context.
   * Called by caller threads
   * @param thread_context thread context to register
   */
  void register_thread_context(std::shared_ptr<ThreadContext> const& thread_context);

  /**
   * Reloads the thread contexts in our local cache.
   * Any invalidated thread contexts with empty queues are removed and any new thread contexts
   * from new threads are added to the returned vector of caches.
   * If there are no invalidated contexts or no new contexts the existing cache is returned
   * @return All current owned thread contexts
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT backend_thread_contexts_cache_t const& backend_thread_contexts_cache();

  /**
   * Clears thread context cache from invalid and empty thread contexts
   */
  QUILL_ATTRIBUTE_HOT void clear_invalid_and_empty_thread_contexts();

private:
  /**
   * Return true if _thread_contexts have changed
   * @note Only accessed by the backend thread
   * @return true if the shared data structure was changed by any calls to Logger
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool _has_new_thread_context() noexcept;

  /**
   * Indicate that the context has changed. A new thread context has been added or removed
   * @note Only called by the caller threads
   */
  void _set_new_thread_context() noexcept;

  /**
   * Increment the counter for a removed thread context. This notifies the backend thread to look for an invalidated context
   */
  void _add_invalid_thread_context() noexcept;

  /**
   * Reduce the value of thread context removed counter. This is decreased by the backend thread
   * when we found and removed the invalided context
   */
  void _sub_invalid_thread_context() noexcept;

  /**
   * @return True if there is an invalid thread context
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool _has_invalid_thread_context() const noexcept;

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
   * Looks into the _thread_context_cache and removes all thread contexts that are 1) invalidated
   * and 2) have an empty queue of no events to process
   *
   * @note Only called by the backend thread
   */
  void _find_and_remove_invalidated_thread_contexts();

private:
  Config const& _config; /**< reference to config */

  Spinlock _spinlock; /**< Protect access when register contexts or removing contexts */
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
  alignas(CACHELINE_SIZE) std::atomic<bool> _new_thread_context{false};

  /**<
   * Indicator of how many thread contexts are removed, if this number is not zero we will search for invalidated and empty
   * queue context until we find it to remove it.
   * Incremented by any thread on thread local destruction, decremented by the backend thread
   */
  alignas(CACHELINE_SIZE) std::atomic<uint8_t> _invalid_thread_context{0};
  char _pad0[detail::CACHELINE_SIZE - sizeof(std::atomic<uint8_t>)] = "\0";
};
} // namespace detail
} // namespace quill