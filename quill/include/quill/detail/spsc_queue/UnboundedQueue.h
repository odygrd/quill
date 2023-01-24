/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>

#include "BoundedQueue.h"

namespace quill::detail
{
/**
 * A singe-producer single-consumer FIFO circular buffer
 *
 * The buffer allows producing and consuming objects
 *
 * Production is wait free.
 *
 * When the internal circlular buffer becomes full a new one will be created and the production
 * will continue in the new buffer.
 *
 * Consumption is wait free. If not data is available a special value is returned. If a new
 * buffer is created from the producer the consumer first consumes everything in the old
 * buffer and then moves to the new buffer.
 */
class UnboundedQueue
{
public:
  using bounded_queue_t = BoundedQueue;

private:
  /** Private Definitions **/

  /**
   * A node has a buffer and a pointer to the next node
   */
  struct alignas(CACHE_LINE_ALIGNED) Node
  {
    /**
     * Constructor
     * @param capacity the capacity of the fixed buffer
     */
    explicit Node(uint32_t bounded_queue_capacity) : bounded_queue(bounded_queue_capacity) {}

    /**
     * Alignment requirement as we have bounded_queue as member
     * @param i i
     * @return allocated memory pointer
     */
    void* operator new(size_t i) { return aligned_alloc(CACHE_LINE_ALIGNED, i); }
    void operator delete(void* p) { aligned_free(p); }

    /** members */
    std::atomic<Node*> next{nullptr};
    bounded_queue_t bounded_queue;
  };

public:
  /**
   * Constructor
   */
  explicit UnboundedQueue(uint32_t initial_bounded_queue_capacity)
    : _producer(new Node(initial_bounded_queue_capacity)), _consumer(_producer)
  {
  }

  /**
   * Deleted
   */
  UnboundedQueue(UnboundedQueue const&) = delete;
  UnboundedQueue& operator=(UnboundedQueue const&) = delete;

  /**
   * Destructor
   */
  ~UnboundedQueue()
  {
    // Get the current consumer node
    Node* current_node = _consumer;

    // Look for extra nodes to delete
    while (current_node != nullptr)
    {
      auto to_delete = current_node;
      current_node = current_node->next;
      delete to_delete;
    }
  }

  /**
   * Reserve contiguous space for the producer without
   * making it visible to the consumer.
   * @return a valid point to the buffer
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_write(uint32_t nbytes)
  {
    // Try to reserve the bounded queue
    std::byte* write_pos = _producer->bounded_queue.prepare_write(nbytes);

    if (QUILL_LIKELY(write_pos != nullptr))
    {
      return write_pos;
    }

    // Then it means the queue doesn't have enough size
    uint32_t capacity = _producer->bounded_queue.capacity() * 2;
    while (capacity < (nbytes + 1))
    {
      capacity = capacity * 2;
    }

    // commit previous write to the old queue before switching
    _producer->bounded_queue.commit_write();

    // We failed to reserve because the queue was full, create a new node with a new queue
    auto next_node = new Node{capacity};

    // store the new node pointer as next in the current node
    _producer->next.store(next_node, std::memory_order_release);

    // producer is now using the next node
    _producer = next_node;

    // reserve again, this time we know we will always succeed, cast to void* to ignore
    write_pos = _producer->bounded_queue.prepare_write(nbytes);
    assert(write_pos && "Already reserved a queue with that capacity");

    return write_pos;
  }

  /**
   * Complement to reserve producer space that makes nbytes starting
   * from the return of reserve producer space visible to the consumer.
   */
  QUILL_ALWAYS_INLINE_HOT void finish_write(uint32_t nbytes)
  {
    _producer->bounded_queue.finish_write(nbytes);
  }

  /**
   * Commit the write to notify the consumer bytes are ready to read
   */
  QUILL_ALWAYS_INLINE_HOT void commit_write() { _producer->bounded_queue.commit_write(); }

  /**
   * Prepare to read from the buffer
   * @return a pair of the buffer location to read and the number of available bytes
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_read()
  {
    std::byte* read_pos = _consumer->bounded_queue.prepare_read();

    if (!read_pos)
    {
      // the buffer is empty check if another buffer exists
      Node* const next_node = _consumer->next.load(std::memory_order_acquire);

      if (QUILL_UNLIKELY(next_node != nullptr))
      {
        // a new buffer was added by the producer, this happens only when we have allocated a new queue

        // try the existing buffer once more
        read_pos = _consumer->bounded_queue.prepare_read();

        if (!read_pos)
        {
          // commit the previous reads before deleting the queue
          _consumer->bounded_queue.commit_read();

          // switch to the new buffer, existing one is deleted
          delete _consumer;
          _consumer = next_node;
          read_pos = _consumer->bounded_queue.prepare_read();
        }
      }
    }
    return read_pos;
  }

  /**
   * Consumes the next nbytes in the buffer and frees it back
   * for the producer to reuse.
   */
  QUILL_ALWAYS_INLINE_HOT void finish_read(uint32_t nbytes)
  {
    _consumer->bounded_queue.finish_read(nbytes);
  }

  /**
   * Commit the read to indicate that the bytes are read and are now free to be reused
   */
  QUILL_ALWAYS_INLINE_HOT void commit_read() { _consumer->bounded_queue.commit_read(); }

  /**
   * Return the current buffer's capacity
   * @return capacity
   */
  QUILL_NODISCARD uint32_t capacity() const noexcept { return _producer->bounded_queue.capacity(); }

  /**
   * checks if the queue is empty
   * @return true if empty, false otherwise
   */
  QUILL_NODISCARD bool empty() const noexcept
  {
    return _consumer->bounded_queue.empty() && (_consumer->next.load(std::memory_order_relaxed) == nullptr);
  }

private:
  /** Modified by either the producer or consumer but never both */
  alignas(CACHE_LINE_ALIGNED) Node* _producer{nullptr};
  alignas(CACHE_LINE_ALIGNED) Node* _consumer{nullptr};
};

} // namespace quill::detail
