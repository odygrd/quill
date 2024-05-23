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
#include <utility>

namespace quill::detail
{
/***/
template <typename T1, typename T2>
struct ArgSizeCalculator<std::pair<T1, T2>>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, std::pair<T1, T2> const& arg) noexcept
  {
    return ArgSizeCalculator<T1>::calculate(conditional_arg_size_cache, arg.first) +
      ArgSizeCalculator<T2>::calculate(conditional_arg_size_cache, arg.second);
  }
};

/***/
template <typename T1, typename T2>
struct Encoder<std::pair<T1, T2>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::pair<T1, T2> const& arg) noexcept
  {
    Encoder<T1>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.first);
    Encoder<T2>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.second);
  }
};

/***/
template <typename T1, typename T2>
#if defined(_WIN32)
struct Decoder<
  std::pair<T1, T2>,
  std::enable_if_t<std::negation_v<std::disjunction<
    std::is_same<T1, wchar_t*>, std::is_same<T1, wchar_t const*>, std::is_same<T1, std::wstring>, std::is_same<T1, std::wstring_view>,
    std::is_same<T2, wchar_t*>, std::is_same<T2, wchar_t const*>, std::is_same<T2, std::wstring>, std::is_same<T2, std::wstring_view>>>>>
#else
struct Decoder<std::pair<T1, T2>>
#endif
{
  static std::pair<T1, T2> decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    std::pair<T1, T2> arg;

    arg.first = Decoder<T1>::decode(buffer, nullptr);
    arg.second = Decoder<T2>::decode(buffer, nullptr);

    if (args_store)
    {
      args_store->push_back(arg);
    }

    return arg;
  }
};

#if defined(_WIN32)
template <typename T1, typename T2>
struct Decoder<
  std::pair<T1, T2>,
  std::enable_if_t<std::disjunction_v<std::is_same<T1, wchar_t*>, std::is_same<T1, wchar_t const*>, std::is_same<T1, std::wstring>,
                                      std::is_same<T1, std::wstring_view>, std::is_same<T2, wchar_t*>, std::is_same<T2, wchar_t const*>,
                                      std::is_same<T2, std::wstring>, std::is_same<T2, std::wstring_view>>>>
{
  /**
   * Chaining stl types not supported for wstrings so we do not return anything
   */
  static void decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    constexpr bool wide_t1 = std::is_same_v<T1, wchar_t*> || std::is_same_v<T1, wchar_t const*> ||
      std::is_same_v<T1, std::wstring> || std::is_same_v<T1, std::wstring_view>;

    constexpr bool wide_t2 = std::is_same_v<T2, wchar_t*> || std::is_same_v<T2, wchar_t const*> ||
      std::is_same_v<T2, std::wstring> || std::is_same_v<T2, std::wstring_view>;

    if (args_store)
    {
      if constexpr (wide_t1 && !wide_t2)
      {
        std::pair<std::string, T2> arg;

        std::wstring_view v = Decoder<T1>::decode(buffer, nullptr);
        arg.first = utf8_encode(v);

        arg.second = Decoder<T2>::decode(buffer, nullptr);

        args_store->push_back(arg);
      }
      else if constexpr (!wide_t1 && wide_t2)
      {
        std::pair<T1, std::string> arg;

        arg.first = Decoder<T1>::decode(buffer, nullptr);

        std::wstring_view v = Decoder<T2>::decode(buffer, nullptr);
        arg.second = utf8_encode(v);

        args_store->push_back(arg);
      }
      else
      {
        std::wstring_view v1 = Decoder<T1>::decode(buffer, nullptr);
        std::wstring_view v2 = Decoder<T2>::decode(buffer, nullptr);
        args_store->push_back(std::pair<std::string, std::string>{utf8_encode(v1), utf8_encode(v2)});
      }
    }
  }
};
#endif
} // namespace quill::detail