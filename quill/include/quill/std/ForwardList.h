/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"

#include "quill/bundled/fmt/core.h"
#include "quill/bundled/fmt/ranges.h"

#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <vector>

#if defined(_WIN32)
  #include <string>
#endif

namespace quill::detail
{
/***/
template <typename T, typename Allocator>
struct ArgSizeCalculator<std::forward_list<T, Allocator>>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache,
                          std::forward_list<T, Allocator> const& arg) noexcept
  {
    // We need to store the size of the forward_list in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    // Cache the number of elements of the forward list to avoid iterating again when
    // encoding. This information is inserted first to maintain sequence in the cache.
    // It will be replaced with the actual count after calculating the size of elements.
    size_t number_of_elements{0};
    conditional_arg_size_cache.push_back(number_of_elements);
    size_t const index = conditional_arg_size_cache.size() - 1u;

    for (auto const& elem : arg)
    {
      total_size += ArgSizeCalculator<T>::calculate(conditional_arg_size_cache, elem);
      ++number_of_elements;
    }

    conditional_arg_size_cache[index] = number_of_elements;
    return total_size;
  }
};

/***/
template <typename T, typename Allocator>
struct Encoder<std::forward_list<T, Allocator>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::forward_list<T, Allocator> const& arg) noexcept
  {
    // First encode the number of elements of the forward list
    size_t const elems_num = conditional_arg_size_cache[conditional_arg_size_cache_index++];
    Encoder<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elems_num);

    for (auto const& elem : arg)
    {
      Encoder<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem);
    }
  }
};

/***/
template <typename T, typename Allocator>
#if defined(_WIN32)
struct Decoder<std::forward_list<T, Allocator>,
               std::enable_if_t<std::negation_v<std::disjunction<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                                                 std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>>
#else
struct Decoder<std::forward_list<T, Allocator>>
#endif
{
  static std::forward_list<T, Allocator> decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    std::forward_list<T, Allocator> arg;

    // Read the size
    size_t const number_of_elements = Decoder<size_t>::decode(buffer, nullptr);

    if (number_of_elements > 0)
    {
      arg.emplace_front(Decoder<T>::decode(buffer, nullptr));
    }

    for (size_t i = 1; i < number_of_elements; ++i)
    {
      auto it = arg.before_begin();
      for (auto curr = arg.begin(); curr != arg.end(); ++it, ++curr)
      {
        // iterate
      }

      // Insert after the last element
      arg.emplace_after(it, Decoder<T>::decode(buffer, nullptr));
    }

    if (args_store)
    {
      args_store->push_back(arg);
    }

    return arg;
  }
};

#if defined(_WIN32)
/***/
template <typename T, typename Allocator>
struct Decoder<std::forward_list<T, Allocator>,
               std::enable_if_t<std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                                   std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>
{
  /**
   * Chaining stl types not supported for wstrings so we do not return anything
   */
  static void decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    if (args_store)
    {
      // Read the size of the vector
      size_t const number_of_elements = Decoder<size_t>::decode(buffer, nullptr);

      std::vector<std::string> encoded_values;
      encoded_values.reserve(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        std::wstring_view v = Decoder<T>::decode(buffer, nullptr);
        encoded_values.emplace_back(utf8_encode(v));
      }

      args_store->push_back(encoded_values);
    }
  }
};
#endif
} // namespace quill::detail