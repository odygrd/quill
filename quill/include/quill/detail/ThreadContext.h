#pragma once

#include <atomic>
#include <cstdint>
#include <cstdlib>

#include "quill/TweakMe.h"

#include "quill/detail/BoundedSPSCQueue.h"
#include "quill/detail/record/RecordBase.h"

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
  using SPSCQueueT = BoundedSPSCQueue<RecordBase, QUILL_BOUNDED_SPSC_QUEUE_SIZE>;

  /**
   * Constructor
   */
  ThreadContext() = default;

  /**
   * Deleted
   */
  ThreadContext(ThreadContext const&) = delete;
  ThreadContext& operator=(ThreadContext const&) = delete;

  void* operator new(size_t i) 
  { 
#if defined(_WIN32)
    return _aligned_malloc(i, 64);
#else
    // TODO:: memalign on linux
#endif
  }

  void operator delete(void* p)
  { 
#if defined(_WIN32)
    _aligned_free(p);
#else
    // TODO:: memalign on linux
#endif
  }

  /**
   * @return A reference to the single-producer-single-consumer queue
   */
  QUILL_NODISCARD inline SPSCQueueT& spsc_queue() noexcept { return _spsc_queue; }

  /**
   * @return A reference to the single-producer-single-consumer queue const overload
   */
  QUILL_NODISCARD inline SPSCQueueT const& spsc_queue() const noexcept { return _spsc_queue; }

  /**
   * @return The cached thread id value
   */
  QUILL_NODISCARD const char* thread_id() const noexcept { return _thread_id.data(); }

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