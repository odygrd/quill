/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"
#include "quill/std/Pair.h"

#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/ranges.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <memory>
  #include <type_traits>
  #include <unordered_map>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * @note compute_encoded_size() and encode() iterate the container twice in immediate succession
 *       on the frontend hot path, and their traversals must visit the same elements in the same
 *       order. For std::unordered_map / std::unordered_multimap the iteration order is determined
 *       by `Hash(key) % bucket_count`; with the default std::hash this is stable across both
 *       passes because no insert, erase, or rehash happens between them. Non-deterministic custom
 *       hashes (e.g. salted or time-seeded) are not supported — they can change iteration order
 *       between the two passes and silently corrupt the encoded payload.
 */
template <template <typename...> class UnorderedMapType, typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
struct Codec<UnorderedMapType<Key, T, Hash, KeyEqual, Allocator>,
             std::enable_if_t<std::disjunction_v<
               std::is_same<UnorderedMapType<Key, T, Hash, KeyEqual, Allocator>, std::unordered_map<Key, T, Hash, KeyEqual, Allocator>>,
               std::is_same<UnorderedMapType<Key, T, Hash, KeyEqual, Allocator>, std::unordered_multimap<Key, T, Hash, KeyEqual, Allocator>>>>>
{
  static size_t compute_encoded_size(SizeCacheVector& conditional_arg_size_cache,
                                     UnorderedMapType<Key, T, Hash, KeyEqual, Allocator> const& arg)
  {
    // We need to store the size of the set in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    if constexpr (std::conjunction_v<std::is_arithmetic<Key>, std::is_arithmetic<T>>)
    {
      // For built-in types, such as arithmetic or enum types, iteration is unnecessary
      total_size += (sizeof(Key) + sizeof(T)) * arg.size();
    }
    else
    {
      // For other complex types it's essential to determine the exact size of each element.
      // For instance, in the case of a collection of std::string, we need to know the exact size
      // of each string as we will be copying them directly to our queue buffer.
      // Any nested codec sizes pushed into conditional_arg_size_cache are consumed in encode()
      // by iterating this same container instance again without mutation.
      for (auto const& elem : arg)
      {
        total_size += Codec<Key>::compute_encoded_size(conditional_arg_size_cache, elem.first);
        total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, elem.second);
      }
    }

    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.size());

    // This traversal must mirror compute_encoded_size(). For unordered containers that relies on
    // iterating the same container instance without mutation or rehashing between the two passes.
    // Forward elements based on whether the container was passed as rvalue
    if constexpr (std::is_rvalue_reference_v<Arg&&>)
    {
      for (auto&& elem : arg)
      {
        // Key is const in map, so we copy it; value can be moved
        Codec<Key>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                           elem.first);
        Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                         std::move(elem.second));
      }
    }
    else
    {
      for (auto const& elem : arg)
      {
        Codec<Key>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                           elem.first);
        Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem.second);
      }
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
    if constexpr (detail::is_wide_string_v<Key> || detail::is_wide_string_v<T>)
    {
      // Wide strings decode to UTF-8 std::string. Collect the entries in a vector to keep the
      // frontend iteration order, which a container keyed by the converted strings could change.
      using DecodedEntry =
        std::pair<decltype(detail::decode_element<Key>(buffer)), decltype(detail::decode_element<T>(buffer))>;

      size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);
      std::vector<DecodedEntry> decoded;
      decoded.reserve(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        // Brace initialization preserves left-to-right evaluation of the decode calls.
        decoded.push_back(DecodedEntry{detail::decode_element<Key>(buffer), detail::decode_element<T>(buffer)});
      }

      return decoded;
    }
    else
    {
      using ReturnKeyType = decltype(Codec<Key>::decode_arg(buffer));
      using ReturnValueType = decltype(Codec<T>::decode_arg(buffer));
      using ReboundHash =
        typename std::conditional<std::is_same<Hash, std::hash<Key>>::value, std::hash<ReturnKeyType>, Hash>::type;
      using ReboundKeyEqual =
        typename std::conditional<std::is_same<KeyEqual, std::equal_to<Key>>::value, std::equal_to<ReturnKeyType>, KeyEqual>::type;
      using ReboundAllocator =
        typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<ReturnKeyType const, ReturnValueType>>;
      UnorderedMapType<ReturnKeyType, ReturnValueType, ReboundHash, ReboundKeyEqual, ReboundAllocator> arg;

      // Read the size of the set
      size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);
      arg.reserve(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        auto key = Codec<Key>::decode_arg(buffer);
        auto value = Codec<T>::decode_arg(buffer);

        if constexpr (std::is_move_constructible_v<ReturnKeyType> && std::is_move_constructible_v<ReturnValueType>)
        {
          arg.insert({std::move(key), std::move(value)});
        }
        else if constexpr (std::is_move_constructible_v<ReturnKeyType>)
        {
          arg.insert({std::move(key), value});
        }
        else if constexpr (std::is_move_constructible_v<ReturnValueType>)
        {
          arg.insert({key, std::move(value)});
        }
        else
        {
          arg.insert({key, value});
        }
      }

      return arg;

    }
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
