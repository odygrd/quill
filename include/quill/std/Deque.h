/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"
#include "quill/core/Utf8Conv.h"

#include "quill/bundled/fmt/ranges.h"
#include "quill/bundled/fmt/format.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <type_traits>
#include <vector>

#if defined(_WIN32)
  #include <string>
#endif

QUILL_BEGIN_NAMESPACE

template <typename T, typename Allocator>
struct Codec<std::deque<T, Allocator>>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::deque<T, Allocator> const& arg) noexcept
  {
    // We need to store the size of the deque in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    if constexpr (std::disjunction_v<std::is_arithmetic<T>, std::is_enum<T>>)
    {
      // For built-in types, such as arithmetic or enum types, iteration is unnecessary
      total_size += sizeof(T) * arg.size();
    }
    else
    {
      // For other complex types it's essential to determine the exact size of each element.
      // For instance, in the case of a collection of std::string, we need to know the exact size
      // of each string as we will be copying them directly to our queue buffer.
      for (auto const& elem : arg)
      {
        total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, elem);
      }
    }

    return total_size;
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::deque<T, Allocator> const& arg) noexcept
  {
    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.size());

    for (auto const& elem : arg)
    {
      Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem);
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
#if defined(_WIN32)
    // Wide string support
    if constexpr (std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                     std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>)
    {
      // Read the size
      size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);

      std::vector<std::string> arg;
      arg.reserve(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        std::wstring_view const v = Codec<T>::decode_arg(buffer);
        arg.emplace_back(detail::utf8_encode(v));
      }

      return arg;
    }
    else
    {
#endif
      // Read the size
      size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);

      std::deque<T, Allocator> arg;
      arg.resize(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        arg[i] = Codec<T>::decode_arg(buffer);
      }

      return arg;
#if defined(_WIN32)
    }
#endif
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_NAMESPACE