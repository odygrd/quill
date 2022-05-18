/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/spsc_queue/UnboundedQueue.h"
#include <atomic>
#include <cstdint>
#include <cstdlib>

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
#if defined(QUILL_USE_BOUNDED_QUEUE)
  using SPSCQueueT = BoundedQueue<QUILL_QUEUE_CAPACITY>;
#else
  using SPSCQueueT = UnboundedQueue<QUILL_QUEUE_CAPACITY>;
#endif

  /**
   * Constructor
   */
  explicit ThreadContext() = default;

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
  void* operator new(size_t i) { return aligned_alloc(CACHELINE_SIZE, i); }

  /**
   * Operator delete
   * @see operator new
   * @param p pointer to object
   */
  void operator delete(void* p) { aligned_free(p); }

  /**
   * @return A reference to the generic single-producer-single-consumer queue
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT SPSCQueueT& spsc_queue() noexcept { return _spsc_queue; }

  /**
   * @return A reference to the generic single-producer-single-consumer queue const overload
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT SPSCQueueT const& spsc_queue() const noexcept
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

#if defined(QUILL_USE_BOUNDED_QUEUE)
  /**
   * Increments the dropped message counter
   */
  void increment_dropped_message_counter() noexcept
  {
    _dropped_message_counter.fetch_add(1, std::memory_order_relaxed);
  }

  /**
   * If the message counter is greater than zero, this will return the value and reset the counter
   * to 0. Called by the backend worker thread
   * @return current value of the dropped message counter
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t get_and_reset_message_counter() noexcept
  {
    if (QUILL_LIKELY(_dropped_message_counter.load(std::memory_order_relaxed) == 0))
    {
      return 0;
    }
    return _dropped_message_counter.exchange(0, std::memory_order_relaxed);
  }
#endif

private:
  SPSCQueueT _spsc_queue; /** queue for this thread, events are pushed here */
  std::string _thread_id = fmt::format_int(get_thread_id()).str(); /**< cache this thread pid */
  std::string _thread_name = get_thread_name();                    /**< cache this thread name */
  std::atomic<bool> _valid{true}; /**< is this context valid, set by the caller, read by the backend worker thread */

#if defined(QUILL_USE_BOUNDED_QUEUE)
  alignas(CACHELINE_SIZE) std::atomic<size_t> _dropped_message_counter{0};
  char _pad0[detail::CACHELINE_SIZE - sizeof(std::atomic<size_t>)] = "\0";
#endif
};
} // namespace detail