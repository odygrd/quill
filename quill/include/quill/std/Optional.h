/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"

#include "quill/bundled/fmt/core.h"
#include "quill/bundled/fmt/std.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#if defined(_WIN32)
  #include <string>
#endif

namespace quill::detail
{
/***/
template <typename T>
struct ArgSizeCalculator<std::optional<T>>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, std::optional<T> const& arg) noexcept
  {
    // We need to store the size of the vector in the buffer, so we reserve space for it.
    // We add sizeof(bool) bytes to accommodate the size information.
    size_t total_size{sizeof(bool)};

    if (arg.has_value())
    {
      total_size += ArgSizeCalculator<T>::calculate(conditional_arg_size_cache, *arg);
    }

    return total_size;
  }
};

/***/
template <typename T>
struct Encoder<std::optional<T>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::optional<T> const& arg) noexcept
  {
    Encoder<bool>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.has_value());

    if (arg.has_value())
    {
      Encoder<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, *arg);
    }
  }
};

/***/
template <typename T>
#if defined(_WIN32)
struct Decoder<std::optional<T>,
               std::enable_if_t<std::negation_v<std::disjunction<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                                                 std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>>
#else
struct Decoder<std::optional<T>>
#endif
{
  static std::optional<T> decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    std::optional<T> arg{std::nullopt};

    bool const has_value = Decoder<bool>::decode(buffer, nullptr);
    if (has_value)
    {
      arg = Decoder<T>::decode(buffer, nullptr);
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
template <typename T>
struct Decoder<std::optional<T>,
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
      std::string encoded_value{"none"};

      bool const has_value = Decoder<bool>::decode(buffer, nullptr);
      if (has_value)
      {
        std::wstring_view arg = Decoder<T>::decode(buffer, nullptr);
        encoded_value = "optional(\"";
        encoded_value += utf8_encode(arg);
        encoded_value += "\")";
      }

      args_store->push_back(encoded_value);
    }
  }
};
#endif
} // namespace quill::detail