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

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <optional>
  #include <type_traits>
  #include <utility>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename T>
struct Codec<std::optional<T>>
{
private:
  // Strip cv qualifiers so that e.g. std::optional<int const> uses the codec of int
  using UT = detail::remove_cvref_t<T>;

public:
  static size_t compute_encoded_size(SizeCacheVector& conditional_arg_size_cache, std::optional<T> const& arg)
  {
    size_t total_size{sizeof(bool)};

    if (arg.has_value())
    {
      total_size += Codec<UT>::compute_encoded_size(conditional_arg_size_cache, *arg);
    }

    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    Codec<bool>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.has_value());

    if (arg.has_value())
    {
      if constexpr (std::is_rvalue_reference_v<Arg&&>)
      {
        Codec<UT>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                          std::move(*arg));
      }
      else
      {
        Codec<UT>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, *arg);
      }
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
    using ReturnType = decltype(detail::decode_element<UT>(buffer));
    bool const has_value = Codec<bool>::decode_arg(buffer);
    if (has_value)
    {
      auto elem = detail::decode_element<UT>(buffer);

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
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
