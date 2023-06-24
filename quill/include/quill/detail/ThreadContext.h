/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/Fmt.h"
#include "quill/detail/backend/TransitEventBuffer.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/spsc_queue/UnboundedQueue.h"
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <variant>

namespace quill::detail
{
/**
 * Each thread has it's own instance of a ThreadContext class
 *
 * The ThreadContext class stores important information local to each thread and also mainly used by
 * the Logger (caller thread) to push events to a thread local SPSC queue.
 *
 * The backend thread reads all existing ThreadContext class instances and pop the events
 * from each thread queue
 */
class ThreadContext
{
public:
  /**
   * Constructor
   */
  explicit ThreadContext(QueueType queue_type, uint32_t default_queue_capacity,
                         uint32_t initial_transit_event_buffer_capacity, bool huge_pages)
    : _transit_event_buffer(initial_transit_event_buffer_capacity)
  {
    if ((queue_type == QueueType::UnboundedBlocking) ||
        (queue_type == QueueType::UnboundedNoMaxLimit) || (queue_type == QueueType::UnboundedDropping))
    {
      _spsc_queue.emplace<UnboundedQueue>(default_queue_capacity, huge_pages);
    }
    else
    {
      _spsc_queue.emplace<BoundedQueue>(default_queue_capacity, huge_pages);
    }
  }

  /**
   * Deleted
   */
  ThreadContext(ThreadContext const&) = delete;
  ThreadContext& operator=(ThreadContext const&) = delete;

  /**
   * Operator new to align this object to a cache line boundary as we always create it on the heap
   * This object should always be aligned to a cache line as it contains the SPSC queue as a member
   * which has cache line alignment requirements
   * @param i size of object
   * @return a pointer to the allocated object
   */
  void* operator new(size_t i) { return alloc_aligned(i, CACHE_LINE_ALIGNED); }

  /**
   * Operator delete
   * @see operator new
   * @param p pointer to object
   */
  void operator delete(void* p) { free_aligned(p); }

  /**
   * @return A reference to the backend's thread transit event buffer
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT detail::UnboundedTransitEventBuffer& transit_event_buffer() noexcept
  {
    return _transit_event_buffer;
  }

  /**
   * @return A reference to the generic single-producer-single-consumer queue
   */
  template <QueueType queue_type>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::conditional_t<(queue_type == QueueType::UnboundedBlocking) || (queue_type == QueueType::UnboundedNoMaxLimit) ||
                                                         (queue_type == QueueType::UnboundedDropping),
                                                       UnboundedQueue, BoundedQueue>&
  spsc_queue() noexcept
  {
    if constexpr ((queue_type == QueueType::UnboundedBlocking) ||
                  (queue_type == QueueType::UnboundedNoMaxLimit) || (queue_type == QueueType::UnboundedDropping))
    {
      return std::get<UnboundedQueue>(_spsc_queue);
    }
    else
    {
      return std::get<BoundedQueue>(_spsc_queue);
    }
  }

  /**
   * @return A reference to the generic single-producer-single-consumer queue const overload
   */
  template <QueueType queue_type>
  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::conditional_t<(queue_type == QueueType::UnboundedBlocking) || (queue_type == QueueType::UnboundedNoMaxLimit) ||
                                                         (queue_type == QueueType::UnboundedDropping),
                                                       UnboundedQueue, BoundedQueue> const&
  spsc_queue() const noexcept
  {
    if constexpr ((queue_type == QueueType::UnboundedBlocking) ||
                  (queue_type == QueueType::UnboundedNoMaxLimit) || (queue_type == QueueType::UnboundedDropping))
    {
      return std::get<UnboundedQueue>(_spsc_queue);
    }
    else
    {
      return std::get<BoundedQueue>(_spsc_queue);
    }
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::variant<std::monostate, UnboundedQueue, BoundedQueue> const& spsc_queue_variant() const noexcept
  {
    return _spsc_queue;
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::variant<std::monostate, UnboundedQueue, BoundedQueue>& spsc_queue_variant() noexcept
  {
    return _spsc_queue;
  }

  /**
   * @return The cached thread id value
   */
  QUILL_NODISCARD char const* thread_id() const noexcept { return _thread_id.data(); }

  /**
   * The thread_name must be set prior to ThreadContext creation (first call to a log statement on that thread)
   * @return The cached thread name value.
   */
  QUILL_NODISCARD char const* thread_name() const noexcept { return _thread_name.data(); }

  /**
   * Invalidate the context.
   */
  void invalidate() noexcept { _valid.store(false, std::memory_order_relaxed); }

  /**
   * Check is the context was invalidated
   * @return true when the context is valid, false otherwise
   */
  QUILL_NODISCARD bool is_valid() const noexcept { return _valid.load(std::memory_order_relaxed); }

  /**
   * Increments the dropped message counter
   */
  void increment_message_failure_counter() noexcept
  {
    _message_failure_counter.fetch_add(1, std::memory_order_relaxed);
  }

  /**
   * If the message failure counter is greater than zero, this will return the value and reset the
   * counter Called by the backend worker thread
   * @return current value of the message message counter
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t get_and_reset_message_failure_counter() noexcept
  {
    if (QUILL_LIKELY(_message_failure_counter.load(std::memory_order_relaxed) == 0))
    {
      return 0;
    }
    return _message_failure_counter.exchange(0, std::memory_order_relaxed);
  }

private:
  std::variant<std::monostate, UnboundedQueue, BoundedQueue> _spsc_queue; /** queue for this thread, events are pushed here */
  UnboundedTransitEventBuffer _transit_event_buffer;                    /** backend thread buffer */
  std::string _thread_id = fmtquill::format_int(get_thread_id()).str(); /**< cache this thread pid */
  std::string _thread_name = get_thread_name(); /**< cache this thread name */
  std::atomic<bool> _valid{true}; /**< is this context valid, set by the caller, read by the backend worker thread */
  alignas(CACHE_LINE_ALIGNED) std::atomic<size_t> _message_failure_counter{0};
};
} // namespace quill::detail
