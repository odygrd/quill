/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"

QUILL_BEGIN_NAMESPACE

namespace detail
{
template <typename T, size_t N>
class InlinedVector
{
public:
  using value_type = T;
  static_assert(std::is_trivially_copyable_v<value_type>, "value_type must be trivially copyable");

  InlinedVector() noexcept : _storage(), _size(0), _capacity(N) 
  {
    std::memset(_storage.inline_buffer, 0, N * sizeof(value_type));
  }

  ~InlinedVector()
  {
    if (_capacity != N)
    {
      delete[] _storage.heap_buffer;
    }
  }

  /**
   * Deleted
   */
  InlinedVector(InlinedVector const& other) = delete;
  InlinedVector& operator=(InlinedVector const& other) = delete;

  /**
   * Push back a new element
   */
  QUILL_ATTRIBUTE_HOT value_type push_back(value_type value)
  {
    if (_size == _capacity)
    {
      size_t new_capacity = _capacity * 2;
      auto* new_data = new value_type[new_capacity];

      if (_capacity == N)
      {
        // Entering here for the first time, then we copy the inline storage
        std::memcpy(new_data, _storage.inline_buffer, N * sizeof(value_type));
      }
      else
      {
        std::memcpy(new_data, _storage.heap_buffer, _capacity * sizeof(value_type));
        delete[] _storage.heap_buffer;
      }

      _storage.heap_buffer = new_data;
      _capacity = new_capacity;
    }

    if (_capacity == N)
    {
      std::memcpy(_storage.inline_buffer + (_size * sizeof(value_type)), &value, sizeof(value_type));
    }
    else
    {
      _storage.heap_buffer[_size] = value;
    }

    ++_size;
    
    return value;
  }

  /**
   * Access element
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT value_type operator[](size_t index) const noexcept
  {
    assert((index < _size) && "index out of bounds");

    if (_capacity == N)
    {
      value_type value;
      std::memcpy(&value, _storage.inline_buffer + (index * sizeof(value_type)), sizeof(value_type));
      return value;
    }

    return _storage.heap_buffer[index];
  }

  /**
   * Assign value at index
   */
  QUILL_ATTRIBUTE_HOT void assign(size_t index, value_type value) noexcept
  {
    assert((index < _size) && "index out of bounds");

    if (_capacity == N)
    {
      std::memcpy(_storage.inline_buffer + (index * sizeof(value_type)), &value, sizeof(value_type));
    }
    else
    {
      _storage.heap_buffer[index] = value;
    }
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t size() const noexcept { return _size; }
  QUILL_NODISCARD size_t capacity() const noexcept { return _capacity; }
  QUILL_ATTRIBUTE_HOT void clear() noexcept { _size = 0; }

private:
  union Storage
  {
    uint8_t inline_buffer[N * sizeof(value_type)];
    value_type* heap_buffer;

    Storage() noexcept : heap_buffer(nullptr) {}
  } _storage;

  size_t _size{0};
  size_t _capacity{N};
};

/**
 * A vector that stores sizes for specific operations using `uint32_t` to optimize space.
 * The capacity of 12 is chosen to fit within a full cache line for better performance.
 */
using SizeCacheVector = InlinedVector<uint32_t, 12>;
static_assert(sizeof(SizeCacheVector) <= detail::CACHE_LINE_SIZE,
              "SizeCacheVector should not exceed a cache line");
} // namespace detail

QUILL_END_NAMESPACE
