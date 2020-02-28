/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Common.h"

#include "quill/detail/UnboundedSPSCQueue.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/record/RecordBase.h"
#include <atomic>
#include <cstdint>
#include <cstdlib>

namespace quill
{
namespace detail
{
/**
 * Each thread has it's own instance of a ThreadContext class
 *
 * The ThreadContext class stores important information local to each thread and also mainly used by
 * the Logger to push LogRecords to the SPSC queue.
 *
 * The backend thread will read all existing ThreadContext class instances and pop the LogRecords
 * from each queue and flush to all the appropriate handlers
 */
class ThreadContext
{
public:
  using SPSCQueueT = UnboundedSPSCQueue<RecordBase>;

  /**
   * Constructor
   */
  explicit ThreadContext(Config const& config) : _spsc_queue(config.initial_queue_capacity()){};

  /**
   * Deleted
   */
  ThreadContext(ThreadContext const&) = delete;
  ThreadContext& operator=(ThreadContext const&) = delete;

  /**
   * Operator new to align this object to a cache line boundary as we always create it on the heap
   * This object should always be aligned to a cache line as it contains the SPSC queue as a member
   * which has cache line alignment requirements
   * @param i
   * @return
   */
  void* operator new(size_t i) { return aligned_alloc(CACHELINE_SIZE, i); }

  /**
   * Operator delete
   * @see operator new
   * @param p
   */
  void operator delete(void* p) { aligned_free(p); }

  /**
   * @return A reference to the single-producer-single-consumer queue
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT SPSCQueueT& spsc_queue() noexcept { return _spsc_queue; }

  /**
   * @return A reference to the single-producer-single-consumer queue const overload
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
   * Invalidate the context.
   */
  void invalidate() noexcept { _valid.store(false, std::memory_order_relaxed); }

  /**
   * Check is the context was invalidated
   * @return true when the context is valid, false otherwise
   */
  QUILL_NODISCARD bool is_valid() const noexcept { return _valid.load(std::memory_order_relaxed); }

private:
  SPSCQueueT _spsc_queue;                                         /** queue for this thread */
  std::string _thread_id{fmt::format_int(get_thread_id()).str()}; /**< cache this thread pid */
  std::atomic<bool> _valid{true}; /**< is this context valid, set by the caller, read by the backend worker thread */
};
} // namespace detail
} // namespace quill