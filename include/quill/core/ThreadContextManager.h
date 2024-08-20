/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/BoundedSPSCQueue.h"
#include "quill/core/Common.h"
#include "quill/core/InlinedVector.h"
#include "quill/core/Spinlock.h"
#include "quill/core/ThreadUtilities.h"
#include "quill/core/UnboundedSPSCQueue.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/** Forward Declarations **/
class UnboundedTransitEventBuffer;
class BackendWorker;

class ThreadContext
{
private:
  union SpscQueueUnion
  {
    UnboundedSPSCQueue unbounded_spsc_queue;
    BoundedSPSCQueue bounded_spsc_queue;

    SpscQueueUnion() {}
    ~SpscQueueUnion() {}
  };

public:
  /***/
  ThreadContext(QueueType queue_type, uint32_t initial_spsc_queue_capacity, bool huges_pages_enabled)
    : _queue_type(queue_type)
  {
    if (has_unbounded_queue_type())
    {
      new (&_spsc_queue_union.unbounded_spsc_queue)
        UnboundedSPSCQueue{initial_spsc_queue_capacity, huges_pages_enabled};
    }
    else if (has_bounded_queue_type())
    {
      new (&_spsc_queue_union.bounded_spsc_queue)
        BoundedSPSCQueue{initial_spsc_queue_capacity, huges_pages_enabled};
    }
  }

  /***/
  ThreadContext(ThreadContext const&) = delete;
  ThreadContext& operator=(ThreadContext const&) = delete;

  /***/
  ~ThreadContext()
  {
    if (has_unbounded_queue_type())
    {
      _spsc_queue_union.unbounded_spsc_queue.~UnboundedSPSCQueue();
    }
    else if (has_bounded_queue_type())
    {
      _spsc_queue_union.bounded_spsc_queue.~BoundedSPSCQueue();
    }
  }

  /***/
  template <QueueType queue_type>
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT
    std::conditional_t<(queue_type == QueueType::UnboundedBlocking) || (queue_type == QueueType::UnboundedUnlimited) ||
                         (queue_type == QueueType::UnboundedDropping),
                       UnboundedSPSCQueue, BoundedSPSCQueue>&
    get_spsc_queue() noexcept
  {
    assert((_queue_type == queue_type) && "ThreadContext queue_type mismatch");

    if constexpr ((queue_type == QueueType::UnboundedBlocking) ||
                  (queue_type == QueueType::UnboundedUnlimited) || (queue_type == QueueType::UnboundedDropping))
    {
      return _spsc_queue_union.unbounded_spsc_queue;
    }
    else
    {
      return _spsc_queue_union.bounded_spsc_queue;
    }
  }

  /***/
  template <QueueType queue_type>
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT
    std::conditional_t<(queue_type == QueueType::UnboundedBlocking) || (queue_type == QueueType::UnboundedUnlimited) ||
                         (queue_type == QueueType::UnboundedDropping),
                       UnboundedSPSCQueue, BoundedSPSCQueue> const&
    get_spsc_queue() const noexcept
  {
    assert((_queue_type == queue_type) && "ThreadContext queue_type mismatch");

    if constexpr ((queue_type == QueueType::UnboundedBlocking) ||
                  (queue_type == QueueType::UnboundedUnlimited) || (queue_type == QueueType::UnboundedDropping))
    {
      return _spsc_queue_union.unbounded_spsc_queue;
    }
    else
    {
      return _spsc_queue_union.bounded_spsc_queue;
    }
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT SizeCacheVector& get_conditional_arg_size_cache() noexcept
  {
    return _conditional_arg_size_cache;
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool has_bounded_queue_type() const noexcept
  {
    return (_queue_type == QueueType::BoundedBlocking) || (_queue_type == QueueType::BoundedDropping);
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool has_unbounded_queue_type() const noexcept
  {
    return (_queue_type == QueueType::UnboundedBlocking) ||
      (_queue_type == QueueType::UnboundedDropping) || (_queue_type == QueueType::UnboundedUnlimited);
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool has_dropping_queue() const noexcept
  {
    return (_queue_type == QueueType::UnboundedDropping) || (_queue_type == QueueType::BoundedDropping);
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool has_blocking_queue() const noexcept
  {
    return (_queue_type == QueueType::UnboundedBlocking) || (_queue_type == QueueType::BoundedBlocking);
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT SpscQueueUnion const& get_spsc_queue_union() const noexcept
  {
    return _spsc_queue_union;
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT SpscQueueUnion& get_spsc_queue_union() noexcept
  {
    return _spsc_queue_union;
  }

  /***/
  QUILL_NODISCARD std::string_view thread_id() const noexcept { return _thread_id; }

  /***/
  QUILL_NODISCARD std::string_view thread_name() const noexcept { return _thread_name; }

  /***/
  void mark_invalid() noexcept { _valid.store(false, std::memory_order_relaxed); }

  /***/
  QUILL_NODISCARD bool is_valid_context() const noexcept
  {
    return _valid.load(std::memory_order_relaxed);
  }

  /***/
  void increment_failure_counter() noexcept
  {
    _failure_counter.fetch_add(1, std::memory_order_relaxed);
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t get_and_reset_failure_counter() noexcept
  {
    if (QUILL_LIKELY(_failure_counter.load(std::memory_order_relaxed) == 0))
    {
      return 0;
    }
    return _failure_counter.exchange(0, std::memory_order_relaxed);
  }

private:
  friend class detail::BackendWorker;

  SpscQueueUnion _spsc_queue_union;            /**< queue for this thread */
  SizeCacheVector _conditional_arg_size_cache; /**< cache for storing sizes needed for specific operations, such as when calling `strn` functions or when a loop is required e.g. caching the size of a type */
  std::string _thread_id = std::to_string(get_thread_id());           /**< cached thread pid */
  std::string _thread_name = get_thread_name();                       /**< cached thread name */
  std::shared_ptr<UnboundedTransitEventBuffer> _transit_event_buffer; /**< backend thread buffer. this could be unique_ptr but it is shared_ptr because of the forward declaration */
  QueueType _queue_type;
  std::atomic<bool> _valid{true}; /**< is this context valid, set by the frontend, read by the backend thread */
  alignas(CACHE_LINE_ALIGNED) std::atomic<size_t> _failure_counter{0};
};

class ThreadContextManager
{
public:
  /***/
  QUILL_EXPORT static ThreadContextManager& instance() noexcept
  {
    static ThreadContextManager instance;
    return instance;
  }

  /***/
  ThreadContextManager(ThreadContextManager const&) = delete;
  ThreadContextManager& operator=(ThreadContextManager const&) = delete;

  /***/
  template <typename TCallback>
  void for_each_thread_context(TCallback cb)
  {
    LockGuard const lock{_spinlock};

    for (auto const& elem : _thread_contexts)
    {
      cb(elem.get());
    }
  }

  /***/
  void register_thread_context(std::shared_ptr<ThreadContext> const& thread_context)
  {
    _spinlock.lock();
    _thread_contexts.push_back(thread_context);
    _spinlock.unlock();
    _new_thread_context_flag.store(true, std::memory_order_release);
  }

  /***/
  void add_invalid_thread_context() noexcept
  {
    _invalid_thread_context_count.fetch_add(1, std::memory_order_relaxed);
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool has_invalid_thread_context() const noexcept
  {
    // Here we do relaxed because if the value is not zero we will look inside ThreadContext invalid
    // flag that is also a relaxed atomic, and then we will look into the SPSC queue size that is
    // also atomic Even if we don't read everything in order we will check again in the next circle
    return _invalid_thread_context_count.load(std::memory_order_relaxed) != 0;
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool new_thread_context_flag() noexcept
  {
    // Again relaxed memory model as in case it is false we will acquire the lock
    if (_new_thread_context_flag.load(std::memory_order_relaxed))
    {
      // if the variable was updated to true, set it to false,
      // There should not be any race condition here as this is the only place _changed is set to
      // false, and we will return true anyway
      _new_thread_context_flag.store(false, std::memory_order_relaxed);
      return true;
    }
    return false;
  }

  /***/
  void remove_shared_invalidated_thread_context(ThreadContext const* thread_context)
  {
    LockGuard const lock{_spinlock};

    // We could use std::find_if, but since this header is included in Logger.h, which is essential
    // for logging purposes, we aim to minimize the number of includes in that path.
    // Therefore, we implement our own find_if loop here.
    auto thread_context_it = _thread_contexts.end();
    for (auto it = _thread_contexts.begin(); it != _thread_contexts.end(); ++it)
    {
      if (it->get() == thread_context)
      {
        thread_context_it = it;
        break;
      }
    }

    assert(thread_context_it != _thread_contexts.end() &&
           "Attempting to remove a non existent thread context");

    assert(!thread_context_it->get()->is_valid_context() &&
           "Attempting to remove a valid thread context");

#ifndef NDEBUG
    assert(thread_context->has_unbounded_queue_type() || thread_context->has_bounded_queue_type());

    if (thread_context->has_unbounded_queue_type())
    {
      assert(thread_context->get_spsc_queue_union().unbounded_spsc_queue.empty() &&
             "Attempting to remove a thread context with a non empty queue");
    }
    else if (thread_context->has_bounded_queue_type())
    {
      assert(thread_context->get_spsc_queue_union().bounded_spsc_queue.empty() &&
             "Attempting to remove a thread context with a non empty queue");
    }
#endif

    _thread_contexts.erase(thread_context_it);

    // Decrement the counter since we found something to
    _invalid_thread_context_count.fetch_sub(1, std::memory_order_relaxed);
  }

private:
  ThreadContextManager() = default;
  ~ThreadContextManager() = default;

private:
  std::vector<std::shared_ptr<ThreadContext>> _thread_contexts; /**< The registered contexts */
  Spinlock _spinlock; /**< Protect access when register contexts or removing contexts */
  std::atomic<bool> _new_thread_context_flag{false};
  std::atomic<uint8_t> _invalid_thread_context_count{0};
};

class ScopedThreadContext
{
public:
  /***/
  ScopedThreadContext(QueueType queue_type, uint32_t spsc_queue_capacity, bool huge_pages_enabled)
    : _thread_context(std::make_shared<ThreadContext>(queue_type, spsc_queue_capacity, huge_pages_enabled))
  {
#ifndef NDEBUG
    // Thread-local flag to track if an instance has been created for this thread.
    // This ensures that get_local_thread_context() is not called with different template arguments
    // when using custom FrontendOptions. Having multiple thread contexts in a single thread is fine
    // and functional but goes against the design principle of maintaining a single thread context
    // per thread.
    thread_local bool thread_local_instance_created = false;

    assert(!thread_local_instance_created &&
           R"(ScopedThreadContext can only be instantiated once per thread. It appears you may be combining default FrontendOptions with custom FrontendOptions. Ensure only one set of FrontendOptions is used to maintain a single thread context per thread.)");

    thread_local_instance_created = true;
#endif

    ThreadContextManager::instance().register_thread_context(_thread_context);
  }

  /***/
  ScopedThreadContext(ScopedThreadContext const&) = delete;
  ScopedThreadContext& operator=(ScopedThreadContext const&) = delete;

  /***/
  ~ScopedThreadContext() noexcept
  {
    // This destructor will get called when the thread that created this wrapper stops
    // we will only invalidate the thread context
    // The backend thread will empty an invalidated ThreadContext and then remove_file it from
    // the ThreadContextCollection
    // There is only exception for the thread who owns the ThreadContextCollection the
    // main thread. The thread context of the main thread can get deleted before getting invalidated
    _thread_context->mark_invalid();

    // Notify the backend thread that one context has been removed
    ThreadContextManager::instance().add_invalid_thread_context();
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT ThreadContext* get_thread_context() const noexcept
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

/***/
template <typename TFrontendOptions>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT ThreadContext* get_local_thread_context() noexcept
{
  thread_local ScopedThreadContext scoped_thread_context{
    TFrontendOptions::queue_type, TFrontendOptions::initial_queue_capacity, TFrontendOptions::huge_pages_enabled};

  return scoped_thread_context.get_thread_context();
}
} // namespace detail

QUILL_END_NAMESPACE