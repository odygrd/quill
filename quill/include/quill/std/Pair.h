/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/Utf8Conv.h"

#include "quill/bundled/fmt/base.h"
#include "quill/bundled/fmt/ranges.h"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace quill
{
template <typename T1, typename T2>
struct Codec<std::pair<T1, T2>>
{
  static size_t compute_encoded_size(std::vector<size_t>& conditional_arg_size_cache,
                                     std::pair<T1, T2> const& arg) noexcept
  {
    return Codec<T1>::compute_encoded_size(conditional_arg_size_cache, arg.first) +
      Codec<T2>::compute_encoded_size(conditional_arg_size_cache, arg.second);
  }

  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::pair<T1, T2> const& arg) noexcept
  {
    Codec<T1>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.first);
    Codec<T2>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.second);
  }

  static auto decode_arg(std::byte*& buffer)
  {
#if defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<T1, wchar_t*>, std::is_same<T1, wchar_t const*>,
                                     std::is_same<T1, std::wstring>, std::is_same<T1, std::wstring_view>,
                                     std::is_same<T2, wchar_t*>, std::is_same<T2, wchar_t const*>,
                                     std::is_same<T2, std::wstring>, std::is_same<T2, std::wstring_view>>)
    {
      constexpr bool wide_t1 = std::is_same_v<T1, wchar_t*> || std::is_same_v<T1, wchar_t const*> ||
        std::is_same_v<T1, std::wstring> || std::is_same_v<T1, std::wstring_view>;

      constexpr bool wide_t2 = std::is_same_v<T2, wchar_t*> || std::is_same_v<T2, wchar_t const*> ||
        std::is_same_v<T2, std::wstring> || std::is_same_v<T2, std::wstring_view>;

      if constexpr (wide_t1 && !wide_t2)
      {
        std::pair<std::string, T2> arg;

        std::wstring_view v = Codec<T1>::decode_arg(buffer);
        arg.first = detail::utf8_encode(v);

        arg.second = Codec<T2>::decode_arg(buffer);

        return arg;
      }
      else if constexpr (!wide_t1 && wide_t2)
      {
        std::pair<T1, std::string> arg;

        arg.first = Codec<T1>::decode_arg(buffer);

        std::wstring_view v = Codec<T2>::decode_arg(buffer);
        arg.second = detail::utf8_encode(v);

        return arg;
      }
      else
      {
        std::pair<std::string, std::string> arg;
        std::wstring_view v1 = Codec<T1>::decode_arg(buffer);
        arg.first = detail::utf8_encode(v1);

        std::wstring_view v2 = Codec<T2>::decode_arg(buffer);
        arg.second = detail::utf8_encode(v2);

        return arg;
      }
    }
    else
    {
#endif

      std::pair<T1, T2> arg;
      arg.first = Codec<T1>::decode_arg(buffer);
      arg.second = Codec<T2>::decode_arg(buffer);
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
} // namespace quill