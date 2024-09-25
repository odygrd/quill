/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/BoundedSPSCQueue.h"
#include "quill/core/Common.h"
#include "quill/core/MathUtilities.h"
#include "quill/core/QuillError.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * A singe-producer single-consumer FIFO circular buffer
 *
 * The buffer allows producing and consuming objects
 *
 * Production is wait free.
 *
 * When the internal circular buffer becomes full a new one will be created and the production
 * will continue in the new buffer.
 *
 * Consumption is wait free. If not data is available a special value is returned. If a new
 * buffer is created from the producer the consumer first consumes everything in the old
 * buffer and then moves to the new buffer.
 */
class UnboundedSPSCQueue
{
private:
  /**
   * A node has a buffer and a pointer to the next node
   */
  struct Node
  {
    /**
     * Constructor
     * @param bounded_queue_capacity the capacity of the fixed buffer
     * @param huge_pages_enabled enables huge pages
     */
    explicit Node(size_t bounded_queue_capacity, bool huge_pages_enabled)
      : bounded_queue(bounded_queue_capacity, huge_pages_enabled)
    {
    }

    /** members */
    std::atomic<Node*> next{nullptr};
    BoundedSPSCQueue bounded_queue;
  };

public:
  struct ReadResult
  {
    explicit ReadResult(std::byte* read_position) : read_pos(read_position) {}

    std::byte* read_pos;
    size_t previous_capacity{0};
    size_t new_capacity{0};
    bool allocation{false};
  };

  /**
   * Constructor
   */
  explicit UnboundedSPSCQueue(size_t initial_bounded_queue_capacity, bool huges_pages_enabled = false)
    : _producer(new Node(initial_bounded_queue_capacity, huges_pages_enabled)), _consumer(_producer)
  {
  }

  /**
   * Deleted
   */
  UnboundedSPSCQueue(UnboundedSPSCQueue const&) = delete;
  UnboundedSPSCQueue& operator=(UnboundedSPSCQueue const&) = delete;

  /**
   * Destructor
   */
  ~UnboundedSPSCQueue()
  {
    // Get the current consumer node
    Node const* current_node = _consumer;

    // Look for extra nodes to delete
    while (current_node != nullptr)
    {
      auto const to_delete = current_node;
      current_node = current_node->next;
      delete to_delete;
    }
  }

  /**
   * Reserve contiguous space for the producer without
   * making it visible to the consumer.
   * @return a valid point to the buffer
   */
#if defined(_MSC_VER)
  // MSVC doesn't like this as template <QueueType queue_type> when called from Logger, while it compiles on MSVC there will be false positives from clang-tidy
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* prepare_write(size_t nbytes, QueueType queue_type)
#else
  template <QueueType queue_type>
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* prepare_write(size_t nbytes)
#endif
  {
    // Try to reserve the bounded queue
    std::byte* write_pos = _producer->bounded_queue.prepare_write(nbytes);

    if (QUILL_LIKELY(write_pos != nullptr))
    {
      return write_pos;
    }

#if defined(_MSC_VER)
    return _handle_full_queue(nbytes, queue_type);
#else
    return _handle_full_queue<queue_type>(nbytes);
#endif
  }

  /**
   * Complement to reserve producer space that makes nbytes starting
   * from the return of reserve producer space visible to the consumer.
   */
  QUILL_ATTRIBUTE_HOT void finish_write(size_t nbytes) noexcept
  {
    _producer->bounded_queue.finish_write(nbytes);
  }

  /**
   * Commit the write to notify the consumer bytes are ready to read
   */
  QUILL_ATTRIBUTE_HOT void commit_write() noexcept { _producer->bounded_queue.commit_write(); }

  /**
   * Finish and commit write as a single function
   */
  QUILL_ATTRIBUTE_HOT void finish_and_commit_write(size_t nbytes) noexcept
  {
    finish_write(nbytes);
    commit_write();
  }

  /**
   * Prepare to read from the buffer
   * @error_notifier a callback used for notifications to the user
   * @return first: pointer to buffer or nullptr, second: a pair of new_capacity, previous_capacity if an allocation
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT ReadResult prepare_read()
  {
    ReadResult read_result{_consumer->bounded_queue.prepare_read()};

    if (read_result.read_pos != nullptr)
    {
      return read_result;
    }

    // the buffer is empty check if another buffer exists
    Node* const next_node = _consumer->next.load(std::memory_order_acquire);

    if (next_node)
    {
      return _read_next_queue(next_node);
    }

    // Queue is empty and no new queue exists
    return read_result;
  }

  /**
   * Consumes the next nbytes in the buffer and frees it back
   * for the producer to reuse.
   */
  QUILL_ATTRIBUTE_HOT void finish_read(size_t nbytes) noexcept
  {
    _consumer->bounded_queue.finish_read(nbytes);
  }

  /**
   * Commit the read to indicate that the bytes are read and are now free to be reused
   */
  QUILL_ATTRIBUTE_HOT void commit_read() noexcept { _consumer->bounded_queue.commit_read(); }

  /**
   * Return the current buffer's capacity
   * @return capacity
   */
  QUILL_NODISCARD size_t capacity() const noexcept { return _consumer->bounded_queue.capacity(); }

  /**
   * checks if the queue is empty
   * @return true if empty, false otherwise
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool empty() const noexcept
  {
    return _consumer->bounded_queue.empty() && (_consumer->next.load(std::memory_order_relaxed) == nullptr);
  }

private:
  /***/
#if defined(_MSC_VER)
  QUILL_NODISCARD std::byte* _handle_full_queue(size_t nbytes, QueueType queue_type)
#else
  template <QueueType queue_type>
  QUILL_NODISCARD std::byte* _handle_full_queue(size_t nbytes)
#endif
  {
    // Then it means the queue doesn't have enough size
    size_t capacity = _producer->bounded_queue.capacity() * 2ull;
    while (capacity < (nbytes + 1))
    {
      capacity = capacity * 2ull;
    }

#if defined(_MSC_VER)
    if ((queue_type == QueueType::UnboundedBlocking) || (queue_type == QueueType::UnboundedDropping))
#else
    if constexpr ((queue_type == QueueType::UnboundedBlocking) || (queue_type == QueueType::UnboundedDropping))
#endif
    {
      size_t constexpr max_bounded_queue_size = 2ull * 1024 * 1024 * 1024; // 2 GB

      if (QUILL_UNLIKELY(capacity > max_bounded_queue_size))
      {
        if (nbytes > max_bounded_queue_size)
        {
          QUILL_THROW(QuillError{
            "Logging single messages larger than 2 GB is not supported with the current queue "
            "type. For UnboundedBlocking or UnboundedDropping queues, this limitation applies.\n"
            "To log single messages larger than 2 GB, consider using the UnboundedUnlimited queue "
            "type.\n"
            "Message size: " +
            std::to_string(nbytes) +
            " bytes\n"
            "Required queue capacity: " +
            std::to_string(capacity) +
            " bytes\n"
            "Maximum allowed queue capacity: " +
            std::to_string(max_bounded_queue_size) + " bytes"});
        }

        // we reached the max_bounded_queue_size we won't be allocating more
        // instead return nullptr to block or drop
        return nullptr;
      }
    }

    // else the UnboundedUnlimited queue has no limits

    // commit previous write to the old queue before switching
    _producer->bounded_queue.commit_write();

    // We failed to reserve because the queue was full, create a new node with a new queue
    auto const next_node = new Node{capacity, _producer->bounded_queue.huge_pages_enabled()};

    // store the new node pointer as next in the current node
    _producer->next.store(next_node, std::memory_order_release);

    // producer is now using the next node
    _producer = next_node;

    // reserve again, this time we know we will always succeed, cast to void* to ignore
    std::byte* const write_pos = _producer->bounded_queue.prepare_write(nbytes);

    assert(write_pos && "write_pos is nullptr");

    return write_pos;
  }

  /***/
  QUILL_NODISCARD ReadResult _read_next_queue(Node* next_node)
  {
    // a new buffer was added by the producer, this happens only when we have allocated a new queue

    // try the existing buffer once more
    ReadResult read_result{_consumer->bounded_queue.prepare_read()};

    if (read_result.read_pos)
    {
      return read_result;
    }

    // Switch to the new buffer for reading
    // commit the previous reads before deleting the queue
    _consumer->bounded_queue.commit_read();

    // switch to the new buffer, existing one is deleted
    auto const previous_capacity = _consumer->bounded_queue.capacity();
    delete _consumer;

    _consumer = next_node;
    read_result.read_pos = _consumer->bounded_queue.prepare_read();

    // we switched to a new here, so we store the capacity info to return it
    read_result.allocation = true;
    read_result.new_capacity = _consumer->bounded_queue.capacity();
    read_result.previous_capacity = previous_capacity;

    return read_result;
  }

private:
  /** Modified by either the producer or consumer but never both */
  alignas(CACHE_LINE_ALIGNED) Node* _producer{nullptr};
  alignas(CACHE_LINE_ALIGNED) Node* _consumer{nullptr};
};

} // namespace detail

QUILL_END_NAMESPACE