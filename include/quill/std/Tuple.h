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
#include <tuple>
#include <type_traits>
#include <utility>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename... Types>
struct Codec<std::tuple<Types...>>
{
private:
  using DecodedTuple =
    std::tuple<decltype(Codec<detail::remove_cvref_t<Types>>::decode_arg(std::declval<std::byte*&>()))...>;

  template <size_t... Indices>
  static auto decode_arg_impl(std::byte*& buffer, std::index_sequence<Indices...>)
  {
    // Strip cv qualifiers so const-qualified element types (e.g. std::tuple<int const, ...>)
    // decode through the underlying element codecs, matching compute_encoded_size()/encode()
    // which already decay the element types
    // Brace initialization preserves left-to-right evaluation of the decode calls.
    return DecodedTuple{((void)Indices, Codec<detail::remove_cvref_t<Types>>::decode_arg(buffer))...};
  }

public:
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::tuple<Types...> const& arg)
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

  template <typename Arg>
  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    if constexpr (std::is_rvalue_reference_v<Arg&&>)
    {
      std::apply(
        [&conditional_arg_size_cache, &conditional_arg_size_cache_index, &buffer](auto&&... elems)
        {
          ((Codec<std::decay_t<decltype(elems)>>::encode(
             buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, std::move(elems))),
           ...);
        },
        std::move(arg));
    }
    else
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
  }

  static auto decode_arg(std::byte*& buffer)
  {
    return decode_arg_impl(buffer, std::index_sequence_for<Types...>{});
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    // Validate the object actually passed to fmt. Checking each element separately is too strict:
    // a user-provided formatter for the complete tuple can intentionally support elements that
    // have no standalone formatter. The check lives here rather than at class scope so that
    // decode-only nested use of this codec does not require a formatter for the tuple itself.
    static_assert(fmtquill::is_formattable<DecodedTuple, char>::value,
                  "Codec<std::tuple<Types...>> requires the decoded tuple type to be formattable "
                  "by bundled fmt");

    if constexpr (std::conjunction_v<std::is_move_constructible<decltype(Codec<detail::remove_cvref_t<Types>>::decode_arg(buffer))>...>)
    {
      auto arg = decode_arg(buffer);
      args_store->push_back(std::move(arg));
    }
    else
    {
      auto const arg = decode_arg(buffer);
      args_store->push_back(arg);
    }
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
