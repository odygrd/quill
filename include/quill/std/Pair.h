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
  // std::map's value_type is std::pair<Key const, T>; strip cv qualifiers so the element codecs
  // are instantiated with the underlying types and the decoded pair is assignable
  using UT1 = detail::remove_cvref_t<T1>;
  using UT2 = detail::remove_cvref_t<T2>;

  static auto decode_non_wide_arg(std::byte*& buffer)
  {
    using ReturnType1 = decltype(Codec<UT1>::decode_arg(buffer));
    using ReturnType2 = decltype(Codec<UT2>::decode_arg(buffer));

    // Brace initialization preserves left-to-right evaluation of the decode calls.
    return std::pair<ReturnType1, ReturnType2>{Codec<UT1>::decode_arg(buffer), Codec<UT2>::decode_arg(buffer)};
  }

public:
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::pair<T1, T2> const& arg)
  {
    // Explicitly separate the calls to ensure the order of evaluation is maintained,
    // as the compiler may evaluate the expressions in a different order, leading to side effects.
    size_t total_size = Codec<UT1>::compute_encoded_size(conditional_arg_size_cache, arg.first);
    total_size += Codec<UT2>::compute_encoded_size(conditional_arg_size_cache, arg.second);
    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    if constexpr (std::is_rvalue_reference_v<Arg&&>)
    {
      Codec<UT1>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                         std::move(arg.first));
      Codec<UT2>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                         std::move(arg.second));
    }
    else
    {
      Codec<UT1>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.first);
      Codec<UT2>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.second);
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
#if defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<UT1, wchar_t*>, std::is_same<UT1, wchar_t const*>,
                                     std::is_same<UT1, std::wstring>, std::is_same<UT1, std::wstring_view>,
                                     std::is_same<UT2, wchar_t*>, std::is_same<UT2, wchar_t const*>,
                                     std::is_same<UT2, std::wstring>, std::is_same<UT2, std::wstring_view>>)
    {
      constexpr bool wide_t1 = std::is_same_v<UT1, wchar_t*> || std::is_same_v<UT1, wchar_t const*> ||
        std::is_same_v<UT1, std::wstring> || std::is_same_v<UT1, std::wstring_view>;

      constexpr bool wide_t2 = std::is_same_v<UT2, wchar_t*> || std::is_same_v<UT2, wchar_t const*> ||
        std::is_same_v<UT2, std::wstring> || std::is_same_v<UT2, std::wstring_view>;

      if constexpr (wide_t1 && !wide_t2)
      {
        using ReturnType2 = decltype(Codec<UT2>::decode_arg(buffer));
        return std::pair<std::string, ReturnType2>{
          detail::utf8_encode(Codec<UT1>::decode_arg(buffer)), Codec<UT2>::decode_arg(buffer)};
      }
      else if constexpr (!wide_t1 && wide_t2)
      {
        using ReturnType1 = decltype(Codec<UT1>::decode_arg(buffer));
        return std::pair<ReturnType1, std::string>{
          Codec<UT1>::decode_arg(buffer), detail::utf8_encode(Codec<UT2>::decode_arg(buffer))};
      }
      else
      {
        return std::pair<std::string, std::string>{detail::utf8_encode(Codec<UT1>::decode_arg(buffer)),
                                                   detail::utf8_encode(Codec<UT2>::decode_arg(buffer))};
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
