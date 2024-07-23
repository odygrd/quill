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

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#if defined(_WIN32)
  #include <string>
#endif

namespace quill::detail
{
/** Specialization for arrays of arithmetic types and enums **/
template <typename T>
struct ArgSizeCalculator<
  T, std::enable_if_t<std::conjunction_v<std::is_array<T>, std::disjunction<std::is_arithmetic<remove_cvref_t<std::remove_extent_t<T>>>, std::is_enum<remove_cvref_t<std::remove_extent_t<T>>>>>>>
{
  static size_t calculate(std::vector<size_t>&, T const&) noexcept { return sizeof(T); }
};

/***/
template <typename T>
struct Encoder<
  T, std::enable_if_t<std::conjunction_v<std::is_array<T>, std::disjunction<std::is_arithmetic<remove_cvref_t<std::remove_extent_t<T>>>, std::is_enum<remove_cvref_t<std::remove_extent_t<T>>>>>>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const&, uint32_t&, T const& arg) noexcept
  {
    std::memcpy(buffer, &arg, sizeof(T));
    buffer += sizeof(T);
  }
};

/***/
template <typename T>
struct Decoder<
  T, std::enable_if_t<std::conjunction_v<std::is_array<T>, std::disjunction<std::is_arithmetic<remove_cvref_t<std::remove_extent_t<T>>>, std::is_enum<remove_cvref_t<std::remove_extent_t<T>>>>>>>
{
  static void decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    using TElem = remove_cvref_t<std::remove_extent_t<T>>;
    static constexpr size_t N = std::extent_v<T>;
    using array_t = std::array<TElem, N>;

    array_t arg;

    for (size_t i = 0; i < N; ++i)
    {
      TElem elem;
      std::memcpy(&elem, buffer, sizeof(TElem));
      arg[i] = elem;
      buffer += sizeof(TElem);
    }

    if (args_store)
    {
      args_store->push_back(arg);
    }
  }
};

/***/
template <typename T, size_t N>
struct ArgSizeCalculator<std::array<T, N>>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, std::array<T, N> const& arg) noexcept
  {
    if constexpr (std::disjunction_v<std::is_arithmetic<T>, std::is_enum<T>>)
    {
      // For built-in types, such as arithmetic or enum types, iteration is unnecessary
      return sizeof(T) * N;
    }
    else
    {
      // For other complex types it's essential to determine the exact size of each element.
      // For instance, in the case of a collection of std::string, we need to know the exact size
      // of each string as we will be copying them directly to our queue buffer.
      size_t total_size{0};

      for (auto const& elem : arg)
      {
        total_size += ArgSizeCalculator<T>::calculate(conditional_arg_size_cache, elem);
      }

      return total_size;
    }
  }
};

/***/
template <typename T, size_t N>
struct Encoder<std::array<T, N>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::array<T, N> const& arg) noexcept
  {
    for (auto const& elem : arg)
    {
      Encoder<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem);
    }
  }
};

/***/
template <typename T, size_t N>
#if defined(_WIN32)
struct Decoder<std::array<T, N>,
               std::enable_if_t<std::negation_v<std::disjunction<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                                                 std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>>
#else
struct Decoder<std::array<T, N>>
#endif
{
  static std::array<T, N> decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    std::array<T, N> arg;

    for (size_t i = 0; i < N; ++i)
    {
      arg[i] = Decoder<T>::decode(buffer, nullptr);
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
template <typename T, size_t N>
struct Decoder<std::array<T, N>,
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
      std::vector<std::string> encoded_values;
      encoded_values.reserve(N);

      for (size_t i = 0; i < N; ++i)
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