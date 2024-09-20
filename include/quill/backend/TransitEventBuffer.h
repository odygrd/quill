/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/TransitEvent.h"
#include "quill/bundled/fmt/format.h" // for assert_fail
#include "quill/core/Attributes.h"
#include "quill/core/MathUtilities.h"

#include <cassert>
#include <cstdint>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{

class TransitEventBuffer
{
public:
  explicit TransitEventBuffer(size_t initial_capacity)
    : _capacity(next_power_of_two(initial_capacity)),
      _storage(std::make_unique<TransitEvent[]>(_capacity)),
      _mask(_capacity - 1u)
  {
  }

  TransitEventBuffer(TransitEventBuffer const&) = delete;
  TransitEventBuffer& operator=(TransitEventBuffer const&) = delete;

  // Move constructor
  TransitEventBuffer(TransitEventBuffer&& other) noexcept
    : _capacity(other._capacity),
      _storage(std::move(other._storage)),
      _mask(other._mask),
      _reader_pos(other._reader_pos),
      _writer_pos(other._writer_pos)
  {
    other._capacity = 0;
    other._mask = 0;
    other._reader_pos = 0;
    other._writer_pos = 0;
  }

  // Move assignment operator
  TransitEventBuffer& operator=(TransitEventBuffer&& other) noexcept
  {
    if (this != &other)
    {
      _capacity = other._capacity;
      _storage = std::move(other._storage);
      _mask = other._mask;
      _reader_pos = other._reader_pos;
      _writer_pos = other._writer_pos;

      other._capacity = 0;
      other._mask = 0;
      other._reader_pos = 0;
      other._writer_pos = 0;
    }
    return *this;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* front() noexcept
  {
    if (_reader_pos == _writer_pos)
    {
      return nullptr;
    }
    return &_storage[_reader_pos & _mask];
  }

  QUILL_ATTRIBUTE_HOT void pop_front() noexcept { ++_reader_pos; }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* back() noexcept
  {
    if (_capacity == size())
    {
      // Buffer is full, need to expand
      _expand();
    }
    return &_storage[_writer_pos & _mask];
  }

  QUILL_ATTRIBUTE_HOT void push_back() noexcept { ++_writer_pos; }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t size() const noexcept
  {
    return _writer_pos - _reader_pos;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t capacity() const noexcept { return _capacity; }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool empty() const noexcept
  {
    return _reader_pos == _writer_pos;
  }

private:
  void _expand()
  {
    size_t const new_capacity = _capacity * 2;

    auto new_storage = std::make_unique<TransitEvent[]>(new_capacity);

    // Move existing elements from the old storage to the new storage.
    // Since the buffer is full, this moves all the previous TransitEvents, preserving their order.
    // The reader position and mask are used to handle the circular buffer's wraparound.
    size_t const current_size = size();
    for (size_t i = 0; i < current_size; ++i)
    {
      new_storage[i] = std::move(_storage[(_reader_pos + i) & _mask]);
    }

    _storage = std::move(new_storage);
    _capacity = new_capacity;
    _mask = _capacity - 1;
    _writer_pos = current_size;
    _reader_pos = 0;
  }

  size_t _capacity;
  std::unique_ptr<TransitEvent[]> _storage;
  size_t _mask;
  size_t _reader_pos{0};
  size_t _writer_pos{0};
};

} // namespace detail

QUILL_END_NAMESPACE