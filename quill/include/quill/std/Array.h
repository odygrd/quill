/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/args.h"
#include "quill/bundled/fmt/core.h"
#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

namespace quill::detail
{
/***/
template <typename Arg, size_t N>
struct ArgSizeCalculator<std::array<Arg, N>>
{
  QUILL_ATTRIBUTE_HOT static constexpr size_t calculate(std::vector<size_t>& c_style_string_lengths_cache,
                                                        std::array<Arg, N> const& arg) noexcept
  {
    size_t total_size{0};

    for (auto const& elem : arg)
    {
      total_size += ArgSizeCalculator<Arg>::calculate(c_style_string_lengths_cache, elem);
    }

    return total_size;
  }
};

/***/
template <typename Arg, size_t N>
struct Encoder<std::array<Arg, N>>
{
  QUILL_ATTRIBUTE_HOT static inline void encode(std::byte*& buffer, std::vector<size_t> const& c_style_string_lengths_cache,
                                                uint32_t& c_style_string_lengths_cache_index,
                                                std::array<Arg, N> const& arg) noexcept
  {
    for (auto const& elem : arg)
    {
      Encoder<Arg>::encode(buffer, c_style_string_lengths_cache, c_style_string_lengths_cache_index, elem);
    }
  }
};

/***/
template <typename Arg, size_t N>
struct Decoder<std::array<Arg, N>>
{
  QUILL_ATTRIBUTE_HOT static inline std::array<Arg, N> decode(
    std::byte*& buffer, fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
  {
    std::array<Arg, N> arg;

    for (size_t i = 0; i < N; ++i)
    {
      arg[i] = Decoder<Arg>::decode(buffer, nullptr);
    }

    if (args_store)
    {
      args_store->push_back(arg);
    }

    return arg;
  }
};
} // namespace quill::detail