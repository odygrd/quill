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
#include "quill/core/QuillError.h"

#include <cstddef>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{

class TransitEventBuffer
{
public:
  explicit TransitEventBuffer(size_t initial_capacity)
    : _initial_capacity(next_power_of_two(initial_capacity)), _mask(_initial_capacity - 1u), _storage(_initial_capacity)
  {
  }

  TransitEventBuffer(TransitEventBuffer const&) = delete;
  TransitEventBuffer& operator=(TransitEventBuffer const&) = delete;

  // Move constructor
  TransitEventBuffer(TransitEventBuffer&& other) noexcept
    : _initial_capacity(other._initial_capacity),
      _mask(other._mask),
      _reader_pos(other._reader_pos),
      _writer_pos(other._writer_pos),
      _shrink_requested(other._shrink_requested),
      _storage(std::move(other._storage))
  {
    other._mask = 0;
    other._reader_pos = 0;
    other._writer_pos = 0;
    other._shrink_requested = false;
  }

  // Move assignment operator
  TransitEventBuffer& operator=(TransitEventBuffer&& other) noexcept
  {
    if (this != &other)
    {
      _initial_capacity = other._initial_capacity;
      _storage = std::move(other._storage);
      _mask = other._mask;
      _reader_pos = other._reader_pos;
      _writer_pos = other._writer_pos;
      _shrink_requested = other._shrink_requested;

      other._mask = 0;
      other._reader_pos = 0;
      other._writer_pos = 0;
      other._shrink_requested = false;
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

  /**
   * Expands the buffer when full, so it can throw on allocation failure
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT TransitEvent* back()
  {
    if (capacity() == size())
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

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t capacity() const noexcept { return _storage.size(); }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool empty() const noexcept
  {
    return _reader_pos == _writer_pos;
  }

  void request_shrink() noexcept { _shrink_requested = true; }

  void try_shrink()
  {
    // we only shrink empty buffers
    if (_shrink_requested && empty())
    {
      if (capacity() > _initial_capacity)
      {
        _storage = std::vector<TransitEvent>(_initial_capacity);
        _mask = _initial_capacity - 1;
        _writer_pos = 0;
        _reader_pos = 0;
      }

      _shrink_requested = false;
    }
  }

private:
  void _expand()
  {
    size_t const new_capacity = capacity() * 2;
    size_t const current_size = size();

    std::vector<TransitEvent> new_storage;
    new_storage.reserve(new_capacity);

    // Move-construct the existing elements into the new storage instead of default-constructing
    // every slot and move-assigning over it; each default-constructed TransitEvent heap-allocates
    // a FormatBuffer that the move-assignment would immediately destroy. Since the buffer is
    // full, this moves all the previous TransitEvents, preserving their order. The reader
    // position and mask are used to handle the circular buffer's wraparound.
    for (size_t i = 0; i < current_size; ++i)
    {
      new_storage.emplace_back(std::move(_storage[(_reader_pos + i) & _mask]));
    }

    // Only the new tail slots are default-constructed. This can throw on allocation failure; the
    // caller relies on the already-cached events surviving so it can keep processing them, so
    // move the events back and leave the buffer unchanged before propagating
    QUILL_TRY { new_storage.resize(new_capacity); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      for (size_t i = 0; i < current_size; ++i)
      {
        _storage[(_reader_pos + i) & _mask] = std::move(new_storage[i]);
      }
      throw;
    }
#endif

    _storage = std::move(new_storage);
    _mask = new_capacity - 1;
    _writer_pos = current_size;
    _reader_pos = 0;
  }

  size_t _initial_capacity;
  size_t _mask;
  size_t _reader_pos{0};
  size_t _writer_pos{0};
  bool _shrink_requested{false};

  // _storage is used as a fixed-size array of ring capacity elements (always a power of two,
  // with _mask being capacity - 1). It is never resized in place; expansion and shrinking
  // replace it with a new vector
  std::vector<TransitEvent> _storage;
};

} // namespace detail

QUILL_END_NAMESPACE
