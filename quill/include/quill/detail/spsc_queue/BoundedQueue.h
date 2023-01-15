/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>
#include <utility>

#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"

namespace quill::detail
{
/**
 * Implements a circular Single Producer Single Consumer FIFO queue
 */
class BoundedQueue
{
public:
  BoundedQueue(size_t capacity) : _capacity(capacity)
  {
    _storage = static_cast<std::byte*>(aligned_alloc(CACHELINE_SIZE, _capacity));
    std::memset(_storage, 0, _capacity);

    _end_of_recorded_space = _storage + _capacity;
    _min_free_space = _capacity;
    _producer_pos.store(_storage);
    _consumer_pos.store(_storage);
  }

  ~BoundedQueue() { aligned_free(_storage); }

  /**
   * Attempt to reserve contiguous space for the producer without
   * making it visible to the consumer.
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* prepare_write(size_t nbytes) noexcept
  {
    // Fast in-line path
    if (_min_free_space > nbytes)
    {
      // There's a subtle point here, all the checks for remaining
      // space are strictly < or >, not <= or => because if we allow
      // the record and print positions to overlap, we can't tell
      // if the buffer either completely full or completely empty.
      // Doing this check here ensures that == means completely empty.

      return _producer_pos.load(std::memory_order_relaxed);
    }

    // Slow allocation

    // Since consumerPos can be updated in a different thread, we
    // save a consistent copy of it here to do calculations on
    std::byte* const producer_pos = _producer_pos.load(std::memory_order_relaxed);
    std::byte* const consumer_pos = _consumer_pos.load(std::memory_order_acquire);

    if (producer_pos >= consumer_pos)
    {
      // producer is ahead of the consumer
      // cxxxxxxxxxp0000EOB
      // end_of_buffer = _storage + _capacity;

      // remaining space to the end of the buffer
      _min_free_space = static_cast<size_t>(_storage + _capacity - producer_pos);

      if (_min_free_space > nbytes)
      {
        // we have enough space
        return producer_pos;
      }

      // Prevent the wrap around if it overlaps the two positions because
      // that would imply the buffer is completely empty when it's not.
      if (QUILL_LIKELY(consumer_pos != _storage))
      {
        // Not enough space at the end of the buffer; wrap around
        // Set the end of the buffer
        _end_of_recorded_space = producer_pos;

        // prevents producerPos from updating before endOfRecordedSpace
        // NOTE: we want to release the value of endOfRecordedSpace to the consumer thread
        _producer_pos.store(_storage, std::memory_order_release);

        // now we wrapped around here, so the remaining space will be from consumer pos until start of buffer
        _min_free_space = static_cast<size_t>(consumer_pos - _storage);

        if (_min_free_space > nbytes)
        {
          // we have enough space and the producer is at the start of the buffer
          return _storage;
        }
      }
    }
    else
    {
      // cachedProducerPos < cachedConsumerPos
      // The consumer pos is in front of the producer, we only have limited space in the buffer
      // we can not check until the end of the buffer
      // xxxp000cxxxx
      _min_free_space = static_cast<size_t>(consumer_pos - producer_pos);

      if (_min_free_space > nbytes)
      {
        // we have enough space
        return producer_pos;
      }
    }

    // we do not have enough space
    return nullptr;
  }

  /**
   * Complement to reserve producer space that makes nbytes starting
   * from the return of reserve producer space visible to the consumer.
   */
  QUILL_ALWAYS_INLINE_HOT void commit_write(size_t nbytes) noexcept
  {
    _min_free_space -= nbytes;
    _producer_pos.store(_producer_pos.load(std::memory_order_relaxed) + nbytes, std::memory_order_release);
  }

  /**
   * Prepare to read from the buffer
   * @return a pair of the buffer location to read and the number of available bytes
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::tuple<std::byte*, size_t, bool> prepare_read() noexcept
  {
    // Save a consistent copy of producerPos
    // Prevent reading new producerPos but old endOf...
    std::byte* consumer_pos = _consumer_pos.load(std::memory_order_relaxed);
    std::byte* producer_pos = _producer_pos.load(std::memory_order_acquire);

    size_t bytes_available;

    if (consumer_pos > producer_pos)
    {
      // consumer is ahead of the producer
      // xxxp0000cxxxEOB
      bytes_available = static_cast<size_t>(_end_of_recorded_space - consumer_pos);

      if (bytes_available > 0)
      {
        // There are bytes to read until the end of the buffer, and we also want to notify the
        // use that there are more to read
        return std::tuple<std::byte*, size_t, bool>{consumer_pos, bytes_available, true};
      }

      // Roll over because there is nothing to read until end of buffer
      _consumer_pos.store(_storage, std::memory_order_release);
    }

    // here the consumer is behind the producer
    consumer_pos = _consumer_pos.load(std::memory_order_relaxed);
    bytes_available = static_cast<size_t>(producer_pos - consumer_pos);

    // there won't be more bytes to read as we haven't wrapped around
    return std::tuple<std::byte*, size_t, bool>{consumer_pos, bytes_available, false};
  }

  /**
   * Consumes the next nbytes in the buffer and frees it back
   * for the producer to reuse.
   * nbytes must be less or equal than what is returned by prepare_read().
   */
  QUILL_ALWAYS_INLINE_HOT void finish_read(size_t nbytes) noexcept
  {
    _consumer_pos.store(_consumer_pos.load(std::memory_order_relaxed) + nbytes, std::memory_order_release);
  }

  /**
   * Gives a pointer to producer pos
   * @return producer position
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT std::byte* producer_pos() const noexcept
  {
    return _producer_pos.load(std::memory_order_relaxed);
  }

  QUILL_NODISCARD bool empty() const noexcept
  {
    return _consumer_pos.load(std::memory_order_relaxed) == _producer_pos.load(std::memory_order_relaxed);
  }

  QUILL_NODISCARD size_t capacity() const noexcept { return _capacity; }

protected:
  std::byte* _storage{nullptr};
  size_t _capacity;

  /** Position within storage[] where the producer may place new data **/
  alignas(CACHELINE_SIZE) std::atomic<std::byte*> _producer_pos;

  /**  Marks the end of valid data for the consumer. Set by the producer on a roll-over read by the consumer **/
  std::byte* _end_of_recorded_space;

  /** Lower bound on the number of bytes the producer can allocate w/o rolling over the
   * producerPos or stalling behind the consumer. Only used by the producer **/
  size_t _min_free_space;

  /**
   * Position within the storage buffer where the consumer will consume
   * the next bytes from. This value is only updated by the consumer.
   */
  alignas(CACHELINE_SIZE) std::atomic<std::byte*> _consumer_pos;
  char _pad0[CACHELINE_SIZE - sizeof(std::atomic<std::byte*>)] = "\0";
};
} // namespace quill::detail
