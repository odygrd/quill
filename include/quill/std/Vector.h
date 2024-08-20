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
#include <type_traits>
#include <vector>

QUILL_BEGIN_NAMESPACE

template <typename T, typename Allocator>
struct Codec<std::vector<T, Allocator>>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::vector<T, Allocator> const& arg) noexcept
  {
    // We need to store the size of the vector in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    if constexpr (std::disjunction_v<std::is_arithmetic<T>, std::is_enum<T>>)
    {
      // Built-in types (arithmetic or enum) don't require iteration.
      // Note: This excludes all trivially copyable types; e.g., std::string_view should not fall into this branch.
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
                     uint32_t& conditional_arg_size_cache_index, std::vector<T, Allocator> const& arg) noexcept
  {
    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.size());

    if constexpr (std::disjunction_v<std::is_arithmetic<T>, std::is_enum<T>>)
    {
      // Built-in types (arithmetic or enum) don't require iteration.
      // Note: This excludes all trivially copyable types; e.g., std::string_view should not fall into this branch.
      std::memcpy(buffer, arg.data(), sizeof(T) * arg.size());
      buffer += sizeof(T) * arg.size();
    }
    else
    {
      for (auto const& elem : arg)
      {
        Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem);
      }
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
#if defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                     std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>)
    {
      // Read the size of the vector
      size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);

      std::vector<std::string> encoded_values;
      encoded_values.reserve(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        std::wstring_view v = Codec<T>::decode_arg(buffer);
        encoded_values.emplace_back(detail::utf8_encode(v));
      }

      return encoded_values;
    }
    else
    {
#endif
      std::vector<T, Allocator> arg;

      // Read the size of the vector
      size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);
      arg.reserve(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        arg.emplace_back(Codec<T>::decode_arg(buffer));
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