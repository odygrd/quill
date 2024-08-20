/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"

#include "quill/bundled/fmt/ranges.h"
#include "quill/bundled/fmt/format.h"

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

QUILL_BEGIN_NAMESPACE

template <typename... Types>
struct Codec<std::tuple<Types...>>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::tuple<Types...> const& arg) noexcept
  {
    size_t total_size{0};

    std::apply(
      [&total_size, &conditional_arg_size_cache](auto const&... elems)
      {
        ((total_size += Codec<std::decay_t<decltype(elems)>>::compute_encoded_size(conditional_arg_size_cache, elems)),
         ...);
      },
      arg);

    return total_size;
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::tuple<Types...> const& arg) noexcept
  {
    std::apply(
      [&conditional_arg_size_cache, &conditional_arg_size_cache_index, &buffer](auto const&... elems)
      {
        ((Codec<std::decay_t<decltype(elems)>>::encode(buffer, conditional_arg_size_cache,
                                                       conditional_arg_size_cache_index, elems)),
         ...);
      },
      arg);
  }

  static auto decode_arg(std::byte*& buffer)
  {
    std::tuple<Types...> arg;

    std::apply([&buffer](auto&... elems)
               { ((elems = Codec<std::decay_t<decltype(elems)>>::decode_arg(buffer)), ...); }, arg);

    return arg;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_NAMESPACE