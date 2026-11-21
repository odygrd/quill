/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <cstring>
  #include <type_traits>
#endif

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/QuillError.h"

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Warray-bounds"
  #pragma GCC diagnostic ignored "-Wstringop-overflow"
#elif defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Warray-bounds"
#elif defined(_WIN32) && defined(_MSC_VER)
  #pragma warning(push)
  #pragma warning(disable : 4789)
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

namespace detail
{

template <typename T, size_t N>
class InlinedVector
{
public:
  using value_type = T;
  static_assert(std::is_trivially_copyable_v<value_type>, "value_type must be trivially copyable");

  InlinedVector() noexcept
  {
    for (size_t i = 0; i < _capacity; ++i)
    {
      _storage.inline_buffer[i] = value_type{};
    }
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
   * Ensures storage for at least new_capacity elements without changing the size.
   */
  void reserve(size_t new_capacity)
  {
    if (new_capacity <= _capacity)
    {
      return;
    }

    auto* new_data = new value_type[new_capacity];

    if (_capacity == N)
    {
      std::memcpy(new_data, _storage.inline_buffer, _size * sizeof(value_type));
    }
    else
    {
      std::memcpy(new_data, _storage.heap_buffer, _size * sizeof(value_type));
      delete[] _storage.heap_buffer;
    }

    _storage.heap_buffer = new_data;
    _capacity = new_capacity;
  }

  /**
   * Push back a new element
   */
  QUILL_ATTRIBUTE_HOT value_type push_back(value_type value)
  {
    if (_size == _capacity)
    {
      if (QUILL_UNLIKELY(_capacity > (SIZE_MAX / 2)))
      {
        QUILL_THROW(QuillError{"InlinedVector capacity overflow"});
      }

      size_t const new_capacity = _capacity * 2;
      auto* new_data = new value_type[new_capacity];

      if (_capacity == N)
      {
        // Entering here for the first time, then we copy the inline storage
        // Use memcpy for trivially copyable types to avoid false positive warnings with LTO
        std::memcpy(new_data, _storage.inline_buffer, _size * sizeof(value_type));
      }
      else
      {
        // Use memcpy for trivially copyable types to avoid false positive warnings with LTO
        std::memcpy(new_data, _storage.heap_buffer, _size * sizeof(value_type));
        delete[] _storage.heap_buffer;
      }

      _storage.heap_buffer = new_data;
      _capacity = new_capacity;
    }

    if (_capacity == N)
    {
      _storage.inline_buffer[_size] = value;
    }
    else
    {
      _storage.heap_buffer[_size] = value;
    }

    ++_size;

    return value;
  }

  /**
   * Appends raw bytes, packed into as many elements as needed. Used by codecs that snapshot a
   * value while sizing it and copy the same bytes when encoding.
   */
  void append_bytes(void const* bytes, size_t bytes_size)
  {
    size_t const count = (bytes_size + sizeof(value_type) - 1u) / sizeof(value_type);

    if (count == 0)
    {
      return;
    }

    if ((_capacity - _size) < count)
    {
      size_t const required = _size + count;
      reserve((required > (_capacity * 2u)) ? required : (_capacity * 2u));
    }

    value_type* const dest = ((_capacity == N) ? _storage.inline_buffer : _storage.heap_buffer) + _size;
    dest[count - 1u] = value_type{}; // zero the padding of the last element
    std::memcpy(dest, bytes, bytes_size);
    _size += count;
  }

  /**
   * Access element
   */
  QUILL_ATTRIBUTE_HOT value_type operator[](size_t index) const
  {
    if (QUILL_UNLIKELY(index >= _size))
    {
      QUILL_THROW(QuillError{"index out of bounds"});
    }

    if (_capacity == N)
    {
      return _storage.inline_buffer[index];
    }
    else
    {
      return _storage.heap_buffer[index];
    }
  }

  QUILL_NODISCARD value_type const* data() const noexcept
  {
    return (_capacity == N) ? _storage.inline_buffer : _storage.heap_buffer;
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t size() const noexcept { return _size; }
  QUILL_NODISCARD size_t capacity() const noexcept { return _capacity; }
  QUILL_ATTRIBUTE_HOT void clear() noexcept { _size = 0; }

private:
  union Storage
  {
    value_type inline_buffer[N];
    value_type* heap_buffer;
  } _storage;

  size_t _size{0};
  size_t _capacity{N};
};

} // namespace detail

/**
 * The size cache passed to custom Codec implementations. It stores sizes computed in
 * compute_encoded_size() for encode() to reuse, using `uint32_t` to save space. The capacity of 12
 * is chosen to fit within a full cache line for better performance.
 */
using SizeCacheVector = detail::InlinedVector<uint32_t, 12>;
static_assert(sizeof(SizeCacheVector) <= detail::QUILL_CACHE_LINE_SIZE,
              "SizeCacheVector should not exceed a cache line");

QUILL_END_EXPORT

QUILL_END_NAMESPACE

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#elif defined(__clang__)
  #pragma GCC diagnostic pop
#elif defined(_WIN32) && defined(_MSC_VER)
  #pragma warning(pop)
#endif
