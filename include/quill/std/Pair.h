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
#include "quill/bundled/fmt/ranges.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#if defined(_WIN32)
  #include <string>
  #include <string_view>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename T1, typename T2>
struct Codec<std::pair<T1, T2>>
{
private:
  static auto decode_non_wide_arg(std::byte*& buffer)
  {
    using ReturnType1 = decltype(Codec<T1>::decode_arg(buffer));
    using ReturnType2 = decltype(Codec<T2>::decode_arg(buffer));

    // Brace initialization preserves left-to-right evaluation of the decode calls.
    return std::pair<ReturnType1, ReturnType2>{Codec<T1>::decode_arg(buffer), Codec<T2>::decode_arg(buffer)};
  }

public:
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::pair<T1, T2> const& arg) noexcept
  {
    // Explicitly separate the calls to ensure the order of evaluation is maintained,
    // as the compiler may evaluate the expressions in a different order, leading to side effects.
    size_t total_size = Codec<T1>::compute_encoded_size(conditional_arg_size_cache, arg.first);
    total_size += Codec<T2>::compute_encoded_size(conditional_arg_size_cache, arg.second);
    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg) noexcept
  {
    if constexpr (std::is_rvalue_reference_v<Arg&&>)
    {
      Codec<T1>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                        std::move(arg.first));
      Codec<T2>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                        std::move(arg.second));
    }
    else
    {
      Codec<T1>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.first);
      Codec<T2>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.second);
    }
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
        using ReturnType2 = decltype(Codec<T2>::decode_arg(buffer));
        return std::pair<std::string, ReturnType2>{
          detail::utf8_encode(Codec<T1>::decode_arg(buffer)), Codec<T2>::decode_arg(buffer)};
      }
      else if constexpr (!wide_t1 && wide_t2)
      {
        using ReturnType1 = decltype(Codec<T1>::decode_arg(buffer));
        return std::pair<ReturnType1, std::string>{
          Codec<T1>::decode_arg(buffer), detail::utf8_encode(Codec<T2>::decode_arg(buffer))};
      }
      else
      {
        return std::pair<std::string, std::string>{detail::utf8_encode(Codec<T1>::decode_arg(buffer)),
                                                   detail::utf8_encode(Codec<T2>::decode_arg(buffer))};
      }
    }
    else
    {
#endif
      return decode_non_wide_arg(buffer);

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
