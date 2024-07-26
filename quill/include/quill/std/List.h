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
#include <list>
#include <type_traits>
#include <vector>

#if defined(_WIN32)
  #include <string>
#endif

namespace quill
{
/***/
template <typename T, typename Allocator>
struct ArgSizeCalculator<std::list<T, Allocator>>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, std::list<T, Allocator> const& arg) noexcept
  {
    // We need to store the size of the list in the buffer, so we reserve space for it.
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
        total_size += ArgSizeCalculator<T>::calculate(conditional_arg_size_cache, elem);
      }
    }

    return total_size;
  }
};

/***/
template <typename T, typename Allocator>
struct Encoder<std::list<T, Allocator>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::list<T, Allocator> const& arg) noexcept
  {
    Encoder<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                            arg.size());

    for (auto const& elem : arg)
    {
      Encoder<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem);
    }
  }
};

/***/
template <typename T, typename Allocator>
#if defined(_WIN32)
struct Decoder<std::list<T, Allocator>,
               std::enable_if_t<std::negation_v<std::disjunction<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                                                 std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>>
#else
struct Decoder<std::list<T, Allocator>>
#endif
{
  static std::list<T, Allocator> decode_arg(std::byte*& buffer)
  {
    // Read the size
    size_t const number_of_elements = Decoder<size_t>::decode_arg(buffer);

    std::list<T, Allocator> arg;

    for (size_t i = 0; i < number_of_elements; ++i)
    {
      arg.emplace_back(Decoder<T>::decode_arg(buffer));
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
template <typename T, typename Allocator>
struct Decoder<std::list<T, Allocator>,
               std::enable_if_t<std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                                   std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>
{
  static std::vector<std::string> decode_arg(std::byte*& buffer)
  {
    // Read the size
    size_t const number_of_elements = Decoder<size_t>::decode_arg(buffer);

    std::vector<std::string> arg;
    arg.reserve(number_of_elements);

    for (size_t i = 0; i < number_of_elements; ++i)
    {
      std::wstring_view const v = Decoder<T>::decode_arg(buffer);
      arg.emplace_back(detail::utf8_encode(v));
    }

    return arg;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};
#endif
} // namespace quill