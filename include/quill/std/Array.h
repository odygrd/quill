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

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
  #include <string>
  #include <string_view>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/** Specialization for arrays of arithmetic types and enums, char arrays are handled in Codec.h **/
template <typename T, std::size_t N>
struct Codec<T[N], std::enable_if_t<std::negation_v<std::is_same<T, char>>>>
{
private:
  // When T can be constructed from what Codec<T>::decode_arg returns (e.g. std::string
  // from std::string_view), we materialise the declared element type so that the
  // decoded std::array matches T and can be assigned by decode_members().
  using DecodedElement =
    detail::remove_cvref_t<decltype(Codec<T>::decode_arg(std::declval<std::byte*&>()))>;
  using ArrayElement = std::conditional_t<std::is_constructible_v<T, DecodedElement>, T, DecodedElement>;

  template <size_t... Indices>
  static auto decode_array_impl(std::byte*& buffer, std::index_sequence<Indices...>)
  {
    // Brace initialization preserves left-to-right evaluation of the decode calls.
    return std::array<ArrayElement, N>{{((void)Indices, ArrayElement{Codec<T>::decode_arg(buffer)})...}};
  }

#if defined(_WIN32)
  template <size_t... Indices>
  static auto decode_wide_array_impl(std::byte*& buffer, std::index_sequence<Indices...>)
  {
    return std::vector<std::string>{{((void)Indices, detail::utf8_encode(Codec<T>::decode_arg(buffer)))...}};
  }
#endif

public:
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache, T const (&arg)[N])
  {
    if constexpr (std::is_arithmetic_v<T>)
    {
      // Built-in arithmetic types don't require iteration.
      // Note: Enums are excluded as they may have custom Codecs (e.g., DirectFormatCodec)
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
        total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, elem);
      }

      return total_size;
    }
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    if constexpr (std::is_arithmetic_v<T>)
    {
      // Built-in arithmetic types don't require iteration.
      // Note: Enums are excluded as they may have custom Codecs (e.g., DirectFormatCodec)
      std::memcpy(buffer, &arg, sizeof(T) * N);
      buffer += sizeof(T) * N;
    }
    else
    {
      if constexpr (std::is_rvalue_reference_v<Arg&&>)
      {
        for (size_t i = 0; i < N; ++i)
        {
          Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                           std::move(arg[i]));
        }
      }
      else
      {
        for (size_t i = 0; i < N; ++i)
        {
          Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg[i]);
        }
      }
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
#if defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                     std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>)
    {
      return decode_wide_array_impl(buffer, std::make_index_sequence<N>{});
    }
    else
#endif
    {
      return decode_array_impl(buffer, std::make_index_sequence<N>{});
    }
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

/** Specialization for std::array **/
template <typename T, size_t N>
struct Codec<std::array<T, N>>
{
private:
  // Strip cv qualifiers so const-qualified element types (e.g. std::array<int const, N>) are
  // encoded and decoded through the underlying element codec
  using UT = detail::remove_cvref_t<T>;

  // When UT can be constructed from what Codec<UT>::decode_arg returns (e.g. std::string
  // from std::string_view), we materialise the declared element type so that the
  // decoded std::array matches UT and can be assigned by decode_members().
  using DecodedElement =
    detail::remove_cvref_t<decltype(Codec<UT>::decode_arg(std::declval<std::byte*&>()))>;
  using ArrayElement = std::conditional_t<std::is_constructible_v<UT, DecodedElement>, UT, DecodedElement>;

  template <size_t... Indices>
  static auto decode_array_impl(std::byte*& buffer, std::index_sequence<Indices...>)
  {
    // Brace initialization preserves left-to-right evaluation of the decode calls.
    return std::array<ArrayElement, N>{{((void)Indices, ArrayElement{Codec<UT>::decode_arg(buffer)})...}};
  }

#if defined(_WIN32)
  template <size_t... Indices>
  static auto decode_wide_array_impl(std::byte*& buffer, std::index_sequence<Indices...>)
  {
    return std::vector<std::string>{{((void)Indices, detail::utf8_encode(Codec<UT>::decode_arg(buffer)))...}};
  }
#endif

public:
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::array<T, N> const& arg)
  {
    if constexpr (std::is_arithmetic_v<UT>)
    {
      // Built-in arithmetic types don't require iteration.
      // Note: Enums are excluded as they may have custom Codecs (e.g., DirectFormatCodec)
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
        total_size += Codec<UT>::compute_encoded_size(conditional_arg_size_cache, elem);
      }

      return total_size;
    }
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    if constexpr (std::is_arithmetic_v<UT>)
    {
      // Built-in arithmetic types don't require iteration.
      // Note: Enums are excluded as they may have custom Codecs (e.g., DirectFormatCodec)
      std::memcpy(buffer, arg.data(), sizeof(T) * N);
      buffer += sizeof(T) * N;
    }
    else
    {
      if constexpr (std::is_rvalue_reference_v<Arg&&>)
      {
        for (auto&& elem : arg)
        {
          Codec<UT>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                            std::move(elem));
        }
      }
      else
      {
        for (auto const& elem : arg)
        {
          Codec<UT>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem);
        }
      }
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
#if defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<UT, wchar_t*>, std::is_same<UT, wchar_t const*>,
                                     std::is_same<UT, std::wstring>, std::is_same<UT, std::wstring_view>>)
    {
      return decode_wide_array_impl(buffer, std::make_index_sequence<N>{});
    }
    else
    {
#endif
      return decode_array_impl(buffer, std::make_index_sequence<N>{});

#if defined(_WIN32)
    }
#endif
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
