#pragma once

#include "quill/detail/ThreadContext.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace quill::detail
{
class ThreadContextCollection
{
private:
  /**ad
   * Class used to wrap a ThreadContext. Used as a thread local object.
   * The constructor always registers a new context to the context collection and
   * the destructor always invalidates it.
   * The logging thread will later clean invalidated contexts when the logger thread is done flushing.
   */
  class ThreadContextWrapper
  {
  public:
    /**
     * Constructor
     * Called by each caller thread once
     * Creates a new context and then registers it to the context collection
     * const as we are calling this inside a const function to be able to use the logger
     * inside const functions
     */
    explicit ThreadContextWrapper(ThreadContextCollection const& thread_context_collection);

    /**
     * Destructor. Invalidates the thread context on the thread destruction
     */
    ~ThreadContextWrapper() noexcept;

    /**
     * Get the contents of the pointer
     * @return
     */
    inline ThreadContext* get_thread_context() const noexcept
    {
      assert(_thread_context && "_thread_context can not be null");
      return _thread_context.get();
    }

  private:
    /**< This could be unique_ptr but the thread context of main thread that owns
     * ThreadContextCollection will be destructed last so we use shared_ptr */
    mutable std::shared_ptr<ThreadContext> _thread_context;
  };

public:
  /**
   * Constructor
   */
  ThreadContextCollection() = default;

  /**
   * Destructor
   */
  ~ThreadContextCollection();

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
  inline ThreadContext* get_local_thread_context() const noexcept
  {
    static thread_local ThreadContextWrapper thread_context_wrapper{*this};
    return thread_context_wrapper.get_thread_context();
  }

  /**
   * Register a newly created thread context.
   * Called by caller threads
   * @param thread_context
   */
  void register_thread_context(std::shared_ptr<ThreadContext> const& thread_context) const;

  /**
   * Remove a thread context from our thread context collection
   * Called by the logging thread
   * @param thread_context
   */
  void remove_thread_context(ThreadContext* thread_context);

  /**
   * @return All current owned thread contexts
   */
  std::vector<ThreadContext*> const& get_cached_thread_contexts();

private:
  /**
   * Return true if _thread_contexts have changed
   * @return
   */
  [[nodiscard]] bool _has_changed() const noexcept;

  /**
   * Indicate that the context has changed. A new thread context has been added or removed
   */
  void _set_changed() const noexcept;

private:
  mutable std::mutex _mutex; /**< Protect access when register contexts or removing contexts */
  mutable std::vector<std::shared_ptr<ThreadContext>> _thread_contexts; /**< The registered contexts */

  /**< A reference to the owned thread contexts that we update when there is any change. We do
   * this so the logging thread does not hold the mutex all the time while it is trying to log.
   * Accessed only by the logging thread
   * */
  std::vector<ThreadContext*> _thread_context_cache;

  /**< Indicator that a new context was added or removed, set by caller or logging thread, used by logging thread only */
  mutable std::atomic<bool> _changed{false};
};
} // namespace quill::detail