/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/Filesystem.h"

#include "quill/bundled/fmt/core.h"
#include "quill/bundled/fmt/std.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace quill::detail
{
/***/
template <>
struct ArgSizeCalculator<fs::path>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, fs::path const& arg) noexcept
  {
    if constexpr (std::is_same_v<fs::path::string_type, std::string>)
    {
      return ArgSizeCalculator<std::string>::calculate(conditional_arg_size_cache, arg.string());
    }
#if defined(_WIN32)
    else if constexpr (std::is_same_v<fs::path::string_type, std::wstring>)
    {
      return ArgSizeCalculator<std::wstring>::calculate(conditional_arg_size_cache, arg.wstring());
    }
#endif
  }
};

/***/
template <>
struct Encoder<fs::path>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, fs::path const& arg) noexcept
  {
    if constexpr (std::is_same_v<fs::path::string_type, std::string>)
    {
      Encoder<std::string>::encode(buffer, conditional_arg_size_cache,
                                   conditional_arg_size_cache_index, arg.string());
    }
#if defined(_WIN32)
    else if constexpr (std::is_same_v<fs::path::string_type, std::wstring>)
    {
      Encoder<std::wstring>::encode(buffer, conditional_arg_size_cache,
                                    conditional_arg_size_cache_index, arg.wstring());
    }
#endif
  }
};

/***/
template <>
struct Decoder<fs::path>
{
  static fs::path decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    fs::path arg;

    if constexpr (std::is_same_v<fs::path::string_type, std::string>)
    {
      arg = Decoder<std::string_view>::decode(buffer, nullptr);
    }
#if defined(_WIN32)
    else if constexpr (std::is_same_v<fs::path::string_type, std::wstring>)
    {
      arg = Decoder<std::wstring_view>::decode(buffer, nullptr);
    }
#endif

    if (args_store)
    {
      args_store->push_back(arg);
    }

    return arg;
  }
};
} // namespace quill::detail