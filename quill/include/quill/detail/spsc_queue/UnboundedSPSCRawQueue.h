/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Os.h"
#include "quill/detail/spsc_queue/BoundedSPSCRawQueue.h"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <iostream>

namespace quill
{
namespace detail
{

/**
 * A singe-producer single-consumer FIFO circular buffer
 *
 * The buffer allows producing and consuming objects
 * The object type has to be provided as a template parameter
 *
 * Production is wait free. When the internal circlular buffer becomes full
 * a new one will be created and the production will continue in the new buffer.
 *
 * Consumption is wait free. If not data is available a special value is returned. If a new
 * buffer is created from the producer the consumer first consumes everything in the old
 * buffer and then moves to the new buffer.
 *
 * The old buffer is not used anymore when it becomes empty once.
 *
 * A grow factor of 1.5 is used.
 *
 * @note This class has a slight performance penalty compared to the fixed_circular_buffer
 *       version but it offers extra safety. When performance is important consider using a
 *       large fixed_circular_buffer instead.
 */
template <size_t Capacity>
class UnboundedSPSCRawQueue
{
public:
  using bounded_spsc_queue_t = BoundedSPSCRawQueue<Capacity>;

private:
  /** Private Definitions **/

  /**
   * A node has a buffer and a pointer to the next node
   */
  struct alignas(CACHELINE_SIZE) node
  {
    /**
     * Constructor
     * @param capacity the capacity of the fixed buffer
     */
    explicit node() = default;

    /**
     * Alignment requirement as we have bounded_spsc_queue as member
     * @param i
     * @return
     */
    void* operator new(size_t i) { return aligned_alloc(CACHELINE_SIZE, i); }
    void operator delete(void* p) { aligned_free(p); }

    /** members */
    bounded_spsc_queue_t bounded_spsc_queue;
    alignas(CACHELINE_SIZE) std::atomic<node*> next{nullptr};
    char _pad1[detail::CACHELINE_SIZE - sizeof(std::atomic<node*>)];
  };

public:
  /**
   * Constructor
   * @param capacity The starting capacity
   */
  explicit UnboundedSPSCRawQueue() : _producer(new node()), _consumer(_producer) {}

  /**
   * Deleted
   */
  UnboundedSPSCRawQueue(UnboundedSPSCRawQueue const&) = delete;
  UnboundedSPSCRawQueue& operator=(UnboundedSPSCRawQueue const&) = delete;

  /**
   * Destructor
   */
  ~UnboundedSPSCRawQueue()
  {
    // Get the current consumer node
    node* current_node = _consumer;

    // Look for extra nodes to delete
    while (current_node != nullptr)
    {
      auto to_delete = current_node;
      current_node = current_node->next;
      delete to_delete;
    }
  }

  /**
   * Used if instead of pushing an object we want to get the raw memory inside the buffer to
   * write the object ourselves.
   * This is used for POD only types to directly memcpy them inside the buffer
   * @note: has to be used along with commit_write
   * @param nbytes the size we want to copy
   * @return Buffer begin to write the object
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT unsigned char* prepare_write(size_t nbytes) noexcept
  {
    unsigned char* write_buffer = _producer->bounded_spsc_queue.prepare_write(nbytes);

    // Try to prepare write to the bounded queue
    if (QUILL_UNLIKELY(!write_buffer))
    {
      // We failed to get the buffer because the queue was full, create a new node with the a new queue
      auto next_node = new node{};

      // store the new node pointer as next in the current node
      _producer->next.store(next_node, std::memory_order_release);

      // producer is now using the next node
      _producer = next_node;

      // add the item again, this time we know we will always succeed, cast to void* to ignore
      write_buffer = _producer->bounded_spsc_queue.prepare_write(nbytes);
    }

    return write_buffer;
  }

  /**
   * Must be called after prepare_write to commit the write to the buffer and make the consumer
   * thread aware of the write
   * @note: must be used after prepare_write
   * @param requested_size size of bytes we wrote to the buffer
   */
  QUILL_ALWAYS_INLINE_HOT void commit_write(size_t nbytes) noexcept
  {
    _producer->bounded_spsc_queue.commit_write(nbytes);
  }

  /**
   * Used to read the raw buffer directly. This is used only when we push PODs
   * @return nullptr if nothing to read, else the location of the buffer to read
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::pair<unsigned char*, size_t> prepare_read() noexcept
  {
    auto buffer_span = _consumer->bounded_spsc_queue.prepare_read();

    if (buffer_span.second == 0)
    {
      // the buffer is empty check if another buffer exists
      node* const next_node = _consumer->next.load(std::memory_order_acquire);

      if (QUILL_UNLIKELY(next_node != nullptr))
      {
        // a new buffer was added by the producer, this happens only when we have allocated a new queue

        // try the existing buffer once more
        buffer_span = _consumer->bounded_spsc_queue.prepare_read();

        if (buffer_span.second == 0)
        {
          // switch to the new buffer, existing one is deleted
          delete _consumer;
          _consumer = next_node;
          buffer_span = _consumer->bounded_spsc_queue.prepare_read();
        }
      }
    }

    return buffer_span;
  }

  /**
   * This must be called after reading bytes from the buffer with prepare_read
   * @param read_size the size we read from the buffer
   */
  QUILL_ALWAYS_INLINE_HOT void finish_read(size_t nbytes) noexcept
  {
    _consumer->bounded_spsc_queue.finish_read(nbytes);
  }

  /**
   * Return the current buffer's capacity
   * @return
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::size_t capacity() const noexcept
  {
    return _producer->bounded_spsc_queue.capacity();
  }

  /**
   * checks if the queue is empty
   * @return
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD bool empty() noexcept
  {
    auto buffer_span = prepare_read();
    return buffer_span.second == 0;
  }

private:
  /** Modified by either the producer or consumer but never both */
  alignas(CACHELINE_SIZE) node* _producer{nullptr};
  alignas(CACHELINE_SIZE) node* _consumer{nullptr};
  char _pad0[detail::CACHELINE_SIZE - sizeof(node*)] = "\0";
};

} // namespace detail
} // namespace quill