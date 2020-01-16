#pragma once

#include <atomic>
#include <cstdint>

#include "quill/detail/BoundedSPSCQueue.h"
#include "quill/detail/Message.h"

namespace quill::detail
{

/**
 * Each thread has it's own instance of a ThreadContext class
 *
 * The ThreadContext class stores important information local to each thread and also mainly used
 * to push log messages to the SPSC queue.
 *
 * The logging thread will read all existing ThreadContext classes pop the messages from each queue
 * and flush to all the appropriate sinks
 */
class ThreadContext
{
private:
  using SPSCQueueT = BoundedSPSCQueue<MessageBase, 33554432>;

public:
  /**
   * Constructor
   */
  ThreadContext() = default;

  /**
   * Deleted
   */
  ThreadContext(ThreadContext const&) = delete;
  ThreadContext& operator=(ThreadContext const&) = delete;

  /**
   * @return A reference to the single-producer-single-consumer queue
   */
  [[nodiscard]] inline SPSCQueueT& spsc_queue() noexcept { return _spsc_queue; }

  /**
   * @return The cached thread id value
   */
  [[nodiscard]] uint64_t thread_id() const noexcept { return _thread_id; }

  /**
   * Invalidate the context.
   */
  void invalidate() noexcept { _valid.store(false, std::memory_order_relaxed); }

  /**
   * Check is the context was invalidated
   * @return true when the context is valid, false otherwise
   */
  [[nodiscard]] bool is_valid() const noexcept { return _valid.load(std::memory_order_relaxed); }

private:
  SPSCQueueT _spsc_queue;               /** queue for this thread */
  uint32_t _thread_id{get_thread_id()}; /**< cache this thread pid */
  std::atomic<bool> _valid{true}; /**< is this context valid, set by the caller, read by the logging thread */
};
} // namespace quill::detail