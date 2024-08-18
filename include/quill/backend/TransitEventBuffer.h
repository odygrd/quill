/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/TransitEvent.h"
#include "quill/core/Attributes.h"
#include "quill/core/MathUtils.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{
template <typename T>
class BoundedTransitEventBufferImpl
{
public:
  using integer_type = T;

  /***/
  explicit BoundedTransitEventBufferImpl(integer_type capacity)
    : _capacity(next_power_of_two(capacity)), _mask(static_cast<integer_type>(_capacity - 1u))
  {
    _storage.resize(_capacity);
  }

  /***/
  BoundedTransitEventBufferImpl(BoundedTransitEventBufferImpl const&) = delete;
  BoundedTransitEventBufferImpl& operator=(BoundedTransitEventBufferImpl const&) = delete;

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* front() noexcept
  {
    if (_writer_pos == _reader_pos)
    {
      // empty
      return nullptr;
    }

    return &_storage[static_cast<uint32_t>(_reader_pos & _mask)];
  }

  /***/
  QUILL_ATTRIBUTE_HOT void pop_front() noexcept { ++_reader_pos; }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* back() noexcept
  {
    if (_capacity - size() == 0)
    {
      // is full
      return nullptr;
    }

    return &_storage[static_cast<uint32_t>(_writer_pos & _mask)];
  }

  /***/
  QUILL_ATTRIBUTE_HOT void push_back() noexcept { ++_writer_pos; }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT integer_type size() const noexcept
  {
    return static_cast<integer_type>(_writer_pos - _reader_pos);
  }

  /***/
  QUILL_NODISCARD integer_type capacity() const noexcept
  {
    return static_cast<integer_type>(_capacity);
  }

private:
  integer_type const _capacity;
  integer_type const _mask;
  integer_type _reader_pos{0};
  integer_type _writer_pos{0};
  std::vector<TransitEvent> _storage;
};

using BoundedTransitEventBuffer = BoundedTransitEventBufferImpl<uint32_t>;

class UnboundedTransitEventBuffer
{
public:
  /***/
  struct Node
  {
    /**
     * Constructor
     * @param transit_buffer_capacity the capacity of the fixed buffer
     */
    explicit Node(uint32_t transit_buffer_capacity) : transit_buffer(transit_buffer_capacity) {}

    /** members */
    Node* next{nullptr};
    BoundedTransitEventBuffer transit_buffer;
  };

  /***/
  explicit UnboundedTransitEventBuffer(uint32_t initial_transit_buffer_capacity)
    : _writer(new Node(initial_transit_buffer_capacity)), _reader(_writer)
  {
  }

  /***/
  UnboundedTransitEventBuffer(UnboundedTransitEventBuffer const&) = delete;
  UnboundedTransitEventBuffer& operator=(UnboundedTransitEventBuffer const&) = delete;

  /***/
  ~UnboundedTransitEventBuffer()
  {
    Node const* reader_node = _reader;

    // Look for extra nodes to delete
    while (reader_node)
    {
      auto const to_delete = reader_node;
      reader_node = reader_node->next;
      delete to_delete;
    }
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* front() noexcept
  {
    TransitEvent* next_event = _reader->transit_buffer.front();

    if (!next_event)
    {
      // the buffer is empty check if another buffer exists
      if (QUILL_UNLIKELY(_reader->next != nullptr))
      {
        // a new buffer was added by the producer, this happens only when we have allocated a new queue

        // switch to the new buffer, existing one is deleted
        Node* next_node = _reader->next;
        delete _reader;
        _reader = next_node;
        next_event = _reader->transit_buffer.front();
      }
    }

    return next_event;
  }

  /***/
  QUILL_ATTRIBUTE_HOT void pop_front() noexcept { _reader->transit_buffer.pop_front(); }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* back() noexcept
  {
    // Try to reserve the bounded queue
    TransitEvent* write_event = _writer->transit_buffer.back();

    if (QUILL_UNLIKELY(write_event == nullptr))
    {
      // buffer doesn't have enough space
      uint64_t capacity = static_cast<uint64_t>(_writer->transit_buffer.capacity()) * 2ull;
      uint64_t constexpr max_bounded_queue_capacity =
        (std::numeric_limits<BoundedTransitEventBuffer::integer_type>::max() >> 1) + 1;

      if (QUILL_UNLIKELY(capacity > max_bounded_queue_capacity))
      {
        capacity = max_bounded_queue_capacity;
      }

      auto const new_node = new Node{static_cast<uint32_t>(capacity)};
      _writer->next = new_node;
      _writer = _writer->next;
      write_event = _writer->transit_buffer.back();
    }

    assert(write_event && "Write event is always true");

    return write_event;
  }

  /***/
  QUILL_ATTRIBUTE_HOT void push_back() noexcept { _writer->transit_buffer.push_back(); }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT uint32_t size() const noexcept
  {
    Node const* reader = _reader;

    uint32_t size = reader->transit_buffer.size();

    while (reader->next)
    {
      reader = reader->next;
      size += reader->transit_buffer.size();
    }

    return size;
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool empty() noexcept { return front() ? false : true; }

private:
  Node* _writer{nullptr};
  Node* _reader{nullptr};
};
} // namespace detail

QUILL_END_NAMESPACE