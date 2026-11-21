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
  #include <list>
  #include <memory>
  #include <type_traits>
  #include <utility>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename T, typename Allocator>
struct Codec<std::list<T, Allocator>>
{
  static size_t compute_encoded_size(SizeCacheVector& conditional_arg_size_cache,
                                     std::list<T, Allocator> const& arg)
  {
    // We need to store the size of the list in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    if constexpr (std::is_arithmetic_v<T>)
    {
      // Built-in arithmetic types don't require iteration.
      // Note: Enums are excluded as they may have custom Codecs (e.g., DirectFormatCodec)
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

  template <typename Arg>
  static void encode(std::byte*& buffer, SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.size());

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
    std::list<ReturnType, ReboundAllocator> arg;

    for (size_t i = 0; i < number_of_elements; ++i)
    {
      auto elem = detail::decode_element<T>(buffer);
      if constexpr (std::is_move_constructible_v<ReturnType>)
      {
        arg.emplace_back(std::move(elem));
      }
      else
      {
        arg.emplace_back(elem);
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
