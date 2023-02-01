/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TransitEvent.h"
#include "quill/detail/misc/Attributes.h"
#include <cstdint>
#include <vector>

namespace quill::detail
{
template <typename T>
class BoundedTransitEventBufferImpl
{
public:
  using integer_type = T;

  explicit BoundedTransitEventBufferImpl(integer_type capacity);
  BoundedTransitEventBufferImpl(BoundedTransitEventBufferImpl const&) = delete;
  BoundedTransitEventBufferImpl& operator=(BoundedTransitEventBufferImpl const&) = delete;

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* front() noexcept;
  QUILL_ATTRIBUTE_HOT void pop_front() noexcept;
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* back() noexcept;
  QUILL_ATTRIBUTE_HOT void push_back() noexcept;
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT integer_type size() const noexcept;
  QUILL_NODISCARD integer_type capacity() const noexcept;

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
  struct Node
  {
    /**
     * Constructor
     * @param capacity the capacity of the fixed buffer
     */
    explicit Node(uint32_t transit_buffer_capacity) : transit_buffer(transit_buffer_capacity) {}

    /** members */
    Node* next{nullptr};
    BoundedTransitEventBuffer transit_buffer;
  };

  explicit UnboundedTransitEventBuffer(uint32_t initial_transit_buffer_capacity);
  UnboundedTransitEventBuffer(UnboundedTransitEventBuffer const&) = delete;
  UnboundedTransitEventBuffer& operator=(UnboundedTransitEventBuffer const&) = delete;
  ~UnboundedTransitEventBuffer();

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* front() noexcept;
  QUILL_ATTRIBUTE_HOT void pop_front() noexcept;
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* back() noexcept;
  QUILL_ATTRIBUTE_HOT void push_back() noexcept;
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT uint32_t size() const noexcept;
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool empty() noexcept;

private:
  Node* _writer{nullptr};
  Node* _reader{nullptr};
};
} // namespace quill::detail
