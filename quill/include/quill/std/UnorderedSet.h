/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/Utf8Conv.h"

#include "quill/bundled/fmt/base.h"
#include "quill/bundled/fmt/ranges.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace quill
{
/***/
template <template <typename...> class UnorderedSetType, typename Key, typename Hash, typename KeyEqual, typename Allocator>
struct ArgSizeCalculator<
  UnorderedSetType<Key, Hash, KeyEqual, Allocator>,
  std::enable_if_t<std::disjunction_v<std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_set<Key, Hash, KeyEqual, Allocator>>,
                                      std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_multiset<Key, Hash, KeyEqual, Allocator>>>>>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache,
                          UnorderedSetType<Key, Hash, KeyEqual, Allocator> const& arg) noexcept
  {
    // We need to store the size of the set in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    if constexpr (std::disjunction_v<std::is_arithmetic<Key>, std::is_enum<Key>>)
    {
      // For built-in types, such as arithmetic or enum types, iteration is unnecessary
      total_size += sizeof(Key) * arg.size();
    }
    else
    {
      // For other complex types it's essential to determine the exact size of each element.
      // For instance, in the case of a collection of std::string, we need to know the exact size
      // of each string as we will be copying them directly to our queue buffer.
      for (auto const& elem : arg)
      {
        total_size += ArgSizeCalculator<Key>::calculate(conditional_arg_size_cache, elem);
      }
    }

    return total_size;
  }
};

/***/
template <template <typename...> class UnorderedSetType, typename Key, typename Hash, typename KeyEqual, typename Allocator>
struct Encoder<UnorderedSetType<Key, Hash, KeyEqual, Allocator>,
               std::enable_if_t<std::disjunction_v<
                 std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_set<Key, Hash, KeyEqual, Allocator>>,
                 std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_multiset<Key, Hash, KeyEqual, Allocator>>>>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index,
                     UnorderedSetType<Key, Hash, KeyEqual, Allocator> const& arg) noexcept
  {
    Encoder<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                            arg.size());

    for (auto const& elem : arg)
    {
      Encoder<Key>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem);
    }
  }
};

/***/
template <template <typename...> class UnorderedSetType, typename Key, typename Hash, typename KeyEqual, typename Allocator>
#if defined(_WIN32)
struct Decoder<
  UnorderedSetType<Key, Hash, KeyEqual, Allocator>,
  std::enable_if_t<std::conjunction_v<
    std::disjunction<std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_set<Key, Hash, KeyEqual, Allocator>>,
                     std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_multiset<Key, Hash, KeyEqual, Allocator>>>,
    std::negation<std::disjunction<std::is_same<Key, wchar_t*>, std::is_same<Key, wchar_t const*>, std::is_same<Key, std::wstring>, std::is_same<Key, std::wstring_view>>>>>>
#else
struct Decoder<UnorderedSetType<Key, Hash, KeyEqual, Allocator>,
               std::enable_if_t<std::disjunction_v<
                 std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_set<Key, Hash, KeyEqual, Allocator>>,
                 std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_multiset<Key, Hash, KeyEqual, Allocator>>>>>
#endif
{
  static UnorderedSetType<Key, Hash, KeyEqual, Allocator> decode_arg(std::byte*& buffer)
  {
    UnorderedSetType<Key, Hash, KeyEqual, Allocator> arg;

    // Read the size of the set
    size_t const number_of_elements = Decoder<size_t>::decode_arg(buffer);
    arg.reserve(number_of_elements);

    for (size_t i = 0; i < number_of_elements; ++i)
    {
      arg.emplace(Decoder<Key>::decode_arg(buffer));
    }

    return arg;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

#if defined(_WIN32)
/***/
template <template <typename...> class UnorderedSetType, typename Key, typename Hash, typename KeyEqual, typename Allocator>
struct Decoder<
  UnorderedSetType<Key, Hash, KeyEqual, Allocator>,
  std::enable_if_t<std::conjunction_v<
    std::disjunction<std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_set<Key, Hash, KeyEqual, Allocator>>,
                     std::is_same<UnorderedSetType<Key, Hash, KeyEqual, Allocator>, std::unordered_multiset<Key, Hash, KeyEqual, Allocator>>>,
    std::disjunction<std::is_same<Key, wchar_t*>, std::is_same<Key, wchar_t const*>, std::is_same<Key, std::wstring>, std::is_same<Key, std::wstring_view>>>>>
{
  static std::vector<std::string> decode_arg(std::byte*& buffer)
  {
    // Read the size of the vector
    size_t const number_of_elements = Decoder<size_t>::decode_arg(buffer);

    std::vector<std::string> encoded_values;
    encoded_values.reserve(number_of_elements);

    for (size_t i = 0; i < number_of_elements; ++i)
    {
      std::wstring_view v = Decoder<Key>::decode_arg(buffer);
      encoded_values.emplace_back(detail::utf8_encode(v));
    }

    return encoded_values;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};
#endif
} // namespace quill