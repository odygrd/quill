/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/Utf8Conv.h"

#include "quill/bundled/fmt/base.h"
#include "quill/bundled/fmt/ranges.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
  #include <string>
#endif

QUILL_BEGIN_NAMESPACE

/** Specialization for arrays of arithmetic types and enums **/
template <typename T>
struct Codec<T,
             std::enable_if_t<std::conjunction_v<
               std::is_array<T>, std::disjunction<std::is_arithmetic<detail::remove_cvref_t<std::remove_extent_t<T>>>, std::is_enum<detail::remove_cvref_t<std::remove_extent_t<T>>>>>>>
{
  using element_type = detail::remove_cvref_t<std::remove_extent_t<T>>;
  static constexpr size_t N = std::extent_v<T>;
  using array_type = std::array<element_type, N>;

  static size_t compute_encoded_size(std::vector<size_t>&, T const&) noexcept { return sizeof(T); }

  static void encode(std::byte*& buffer, std::vector<size_t> const&, uint32_t&, T const& arg) noexcept
  {
    std::memcpy(buffer, &arg, sizeof(T));
    buffer += sizeof(T);
  }

  static auto decode_arg(std::byte*& buffer)
  {
    array_type arg;
    for (size_t i = 0; i < N; ++i)
    {
      std::memcpy(&arg[i], buffer, sizeof(element_type));
      buffer += sizeof(element_type);
    }
    return arg;
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
  static size_t compute_encoded_size(std::vector<size_t>& conditional_arg_size_cache,
                                     std::array<T, N> const& arg) noexcept
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
        total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, elem);
      }

      return total_size;
    }
  }

  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::array<T, N> const& arg) noexcept
  {
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
      std::vector<std::string> arg;
      arg.reserve(N);

      for (size_t i = 0; i < N; ++i)
      {
        std::wstring_view v = Codec<T>::decode_arg(buffer);
        arg.emplace_back(detail::utf8_encode(v));
      }

      return arg;
    }
    else
    {
#endif
      std::array<T, N> arg;

      for (size_t i = 0; i < N; ++i)
      {
        arg[i] = Codec<T>::decode_arg(buffer);
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