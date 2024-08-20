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
#include <forward_list>
#include <vector>

#if defined(_WIN32)
  #include <string>
#endif

QUILL_BEGIN_NAMESPACE

template <typename T, typename Allocator>
struct Codec<std::forward_list<T, Allocator>>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::forward_list<T, Allocator> const& arg) noexcept
  {
    // We need to store the size of the forward_list in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    // Cache the number of elements of the forward list to avoid iterating again when
    // encoding. This information is inserted first to maintain sequence in the cache.
    // It will be replaced with the actual count after calculating the size of elements.
    size_t number_of_elements{0};
    conditional_arg_size_cache.push_back(static_cast<uint32_t>(number_of_elements));
    size_t const index = conditional_arg_size_cache.size() - 1u;

    for (auto const& elem : arg)
    {
      total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, elem);
      ++number_of_elements;
    }

    assert((number_of_elements <= std::numeric_limits<uint32_t>::max()) &&
           "len is outside the supported range");
    conditional_arg_size_cache.assign(index, static_cast<uint32_t>(number_of_elements));
    return total_size;
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::forward_list<T, Allocator> const& arg) noexcept
  {
    // First encode the number of elements of the forward list
    size_t const elems_num = conditional_arg_size_cache[conditional_arg_size_cache_index++];
    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elems_num);

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

      std::forward_list<T, Allocator> arg;

      if (number_of_elements > 0)
      {
        arg.emplace_front(Codec<T>::decode_arg(buffer));
      }

      for (size_t i = 1; i < number_of_elements; ++i)
      {
        auto it = arg.before_begin();
        for (auto curr = arg.begin(); curr != arg.end(); ++it, ++curr)
        {
          // iterate
        }

        // Insert after the last element
        arg.emplace_after(it, Codec<T>::decode_arg(buffer));
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