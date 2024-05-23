/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"

#include "quill/bundled/fmt/core.h"
#include "quill/bundled/fmt/ranges.h"

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

namespace quill::detail
{
/***/
template <typename... Types>
struct ArgSizeCalculator<std::tuple<Types...>>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, std::tuple<Types...> const& arg) noexcept
  {
    size_t total_size{0};

    std::apply(
      [&total_size, &conditional_arg_size_cache](auto const&... elems)
      {
        ((total_size += ArgSizeCalculator<std::decay_t<decltype(elems)>>::calculate(conditional_arg_size_cache, elems)),
         ...);
      },
      arg);

    return total_size;
  }
};

/***/
template <typename... Types>
struct Encoder<std::tuple<Types...>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::tuple<Types...> const& arg) noexcept
  {
    std::apply(
      [&conditional_arg_size_cache, &conditional_arg_size_cache_index, &buffer](auto const&... elems)
      {
        ((Encoder<std::decay_t<decltype(elems)>>::encode(buffer, conditional_arg_size_cache,
                                                         conditional_arg_size_cache_index, elems)),
         ...);
      },
      arg);
  }
};

/***/
template <typename... Types>
struct Decoder<std::tuple<Types...>>
{
  static auto decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    std::tuple<Types...> arg{};

    std::apply([&buffer](auto&... elems)
               { ((elems = Decoder<std::decay_t<decltype(elems)>>::decode(buffer, nullptr)), ...); }, arg);

    if (args_store)
    {
      args_store->push_back(arg);
    }

    return arg;
  }
};
} // namespace quill::detail