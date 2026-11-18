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

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <type_traits>
  #include <utility>
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

public:
  static size_t compute_encoded_size(SizeCacheVector& conditional_arg_size_cache,
                                     std::pair<T1, T2> const& arg)
  {
    // Explicitly separate the calls to ensure the order of evaluation is maintained,
    // as the compiler may evaluate the expressions in a different order, leading to side effects.
    size_t total_size = Codec<UT1>::compute_encoded_size(conditional_arg_size_cache, arg.first);
    total_size += Codec<UT2>::compute_encoded_size(conditional_arg_size_cache, arg.second);
    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    if constexpr (std::is_rvalue_reference_v<Arg&&>)
    {
      Codec<UT1>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                         std::get<0>(std::forward<Arg>(arg)));
      Codec<UT2>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                         std::get<1>(std::forward<Arg>(arg)));
    }
    else
    {
      Codec<UT1>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.first);
      Codec<UT2>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.second);
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
    using ReturnType1 = decltype(detail::decode_element<UT1>(buffer));
    using ReturnType2 = decltype(detail::decode_element<UT2>(buffer));

    // Brace initialization preserves left-to-right evaluation of the decode calls.
    return std::pair<ReturnType1, ReturnType2>{detail::decode_element<UT1>(buffer),
                                               detail::decode_element<UT2>(buffer)};
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
