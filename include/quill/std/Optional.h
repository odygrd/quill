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
#include "quill/bundled/fmt/std.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#if defined(_WIN32)
  #include <string>
  #include <string_view>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename T>
struct Codec<std::optional<T>>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::optional<T> const& arg)
  {
    size_t total_size{sizeof(bool)};

    if (arg.has_value())
    {
      total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, *arg);
    }

    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    Codec<bool>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.has_value());

    if (arg.has_value())
    {
      if constexpr (std::is_rvalue_reference_v<Arg&&>)
      {
        Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                         std::move(*arg));
      }
      else
      {
        Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, *arg);
      }
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
#if defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                     std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>)
    {
      std::optional<std::string> arg{std::nullopt};

      bool const has_value = Codec<bool>::decode_arg(buffer);
      if (has_value)
      {
        std::wstring_view const value = Codec<T>::decode_arg(buffer);
        arg.emplace(detail::utf8_encode(value));
      }

      return arg;
    }
    else
    {
#endif
      using ReturnType = decltype(Codec<T>::decode_arg(buffer));
      bool const has_value = Codec<bool>::decode_arg(buffer);
      if (has_value)
      {
        auto elem = Codec<T>::decode_arg(buffer);

        if constexpr (std::is_move_constructible_v<ReturnType>)
        {
          return std::optional<ReturnType>{std::move(elem)};
        }
        else
        {
          return std::optional<ReturnType>{elem};
        }
      }

      return std::optional<ReturnType>{std::nullopt};

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
