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

#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/ranges.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <forward_list>
  #include <memory>
  #include <type_traits>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename T, typename Allocator>
struct Codec<std::forward_list<T, Allocator>>
{
  static size_t compute_encoded_size(SizeCacheVector& conditional_arg_size_cache,
                                     std::forward_list<T, Allocator> const& arg)
  {
    // We need to store the size of the forward_list in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    for (auto const& elem : arg)
    {
      total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, elem);
    }

    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    // First encode the number of elements of the forward list.
    // The wire format stores this as size_t; do not put it in SizeCacheVector, which is uint32_t
    // and is reserved for nested codecs such as strings.
    size_t elems_num{0};
    for (auto const& ignored : arg)
    {
      (void)ignored;
      ++elems_num;
    }

    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elems_num);

    if constexpr (std::is_rvalue_reference_v<Arg&&>)
    {
      for (auto&& elem : arg)
      {
        Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, std::move(elem));
      }
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
    // Read the size
    size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);

    using ReturnType = decltype(detail::decode_element<T>(buffer));
    using ReboundAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<ReturnType>;
    std::forward_list<ReturnType, ReboundAllocator> arg;

    auto last_inserted = arg.before_begin(); // Keeps track of the last inserted position

    for (size_t i = 0; i < number_of_elements; ++i)
    {
      // Insert after the last inserted element and update the iterator
      auto elem = detail::decode_element<T>(buffer);
      if constexpr (std::is_move_constructible_v<ReturnType>)
      {
        last_inserted = arg.emplace_after(last_inserted, std::move(elem));
      }
      else
      {
        last_inserted = arg.emplace_after(last_inserted, elem);
      }
    }

    return arg;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
