/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/DeferredFormatCodec.h"
#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"
#include "quill/core/QuillError.h"

#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/std.h"

#ifndef QUILL_MODULE
  #include <array>
  #include <cstddef>
  #include <cstdint>
  #include <cstdlib>
  #include <optional>
  #include <type_traits>
  #include <utility>
  #include <variant>
#endif

QUILL_BEGIN_NAMESPACE

#if defined(_WIN32) && defined(_MSC_VER)
  // silence msvc warning C4702: unreachable code
  #pragma warning(push)
  #pragma warning(disable : 4702)
#endif

namespace detail
{
/** decode_element() converts wide strings to UTF-8 on Windows, which the narrow variant formatter needs */
template <typename T>
struct DecodedVariantAlternative
{
  using type =
    std::remove_cv_t<std::remove_reference_t<decltype(decode_element<T>(std::declval<std::byte*&>()))>>;
};

template <typename T>
using decoded_variant_alternative_t = typename DecodedVariantAlternative<T>::type;

template <typename... Types>
struct DecodedVariant
{
  using variant_type = std::variant<Types...>;

  /** Single rvalue conversion so `member = decoded` is unambiguous, e.g. in decode_members(). */
  operator variant_type() &&
  {
    if (QUILL_UNLIKELY(!value.has_value()))
    {
      QUILL_THROW(QuillError{"Decoded std::variant is valueless_by_exception"});
    }

    return std::move(*value);
  }

  std::optional<variant_type> value;
};

template <size_t Index, typename DecodedVariantType, typename DecodedType>
QUILL_NODISCARD inline DecodedVariantType make_decoded_variant(DecodedType&& decoded_type)
{
  using StoredType = std::remove_reference_t<DecodedType>;
  using VariantType = typename DecodedVariantType::variant_type;

  if constexpr (std::is_move_constructible_v<StoredType>)
  {
    return DecodedVariantType{std::optional<VariantType>{std::in_place, std::in_place_index<Index>,
                                                         std::forward<DecodedType>(decoded_type)}};
  }
  else
  {
    return DecodedVariantType{std::optional<VariantType>{std::in_place, std::in_place_index<Index>, decoded_type}};
  }
}

} // namespace detail

QUILL_BEGIN_EXPORT

template <>
struct Codec<std::monostate>
{
  static size_t compute_encoded_size(SizeCacheVector& conditional_arg_size_cache, std::monostate const& arg) noexcept
  {
    (void)conditional_arg_size_cache;
    (void)arg;
    return 0;
  }

  static void encode(std::byte*& buffer, SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::monostate const& arg) noexcept
  {
    (void)buffer;
    (void)conditional_arg_size_cache;
    (void)conditional_arg_size_cache_index;
    (void)arg;
  }

  static std::monostate decode_arg(std::byte*& buffer)
  {
    (void)buffer;
    return std::monostate{};
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    (void)buffer;
    args_store->push_back(std::monostate{});
  }
};

template <typename... Types>
struct Codec<std::variant<Types...>>
{
private:
  using VariantType = std::variant<Types...>;
  using DecodedVariantValueType = std::variant<detail::decoded_variant_alternative_t<Types>...>;
  using DecodedVariantType = detail::DecodedVariant<detail::decoded_variant_alternative_t<Types>...>;
  static constexpr bool can_preserve_valueless_state = std::is_same_v<DecodedVariantValueType, VariantType>;

  static_assert(fmtquill::detail::is_variant_formattable<DecodedVariantValueType, char>::value,
                "Codec<std::variant<Types...>> requires the decoded variant type to be formattable "
                "by bundled fmt.");

  template <size_t Index>
  static DecodedVariantType decode_alternative(std::byte*& buffer)
  {
    using OriginalType = std::variant_alternative_t<Index, VariantType>;
    auto decoded_value = detail::decode_element<OriginalType>(buffer);
    return detail::make_decoded_variant<Index, DecodedVariantType>(std::move(decoded_value));
  }

  template <size_t... Indices>
  static DecodedVariantType decode_by_index(std::byte*& buffer, size_t active_index,
                                            std::index_sequence<Indices...>)
  {
    using DecodeFn = DecodedVariantType (*)(std::byte*&);
    static constexpr std::array<DecodeFn, sizeof...(Types)> decode_fns{{&decode_alternative<Indices>...}};
    return decode_fns[active_index](buffer);
  }

public:
  static size_t compute_encoded_size(SizeCacheVector& conditional_arg_size_cache, VariantType const& arg)
  {
    size_t total_size{sizeof(size_t)};

    if (arg.valueless_by_exception())
    {
      if constexpr (can_preserve_valueless_state)
      {
        total_size += DeferredFormatCodec<VariantType>::compute_encoded_size(conditional_arg_size_cache, arg);
      }

      return total_size;
    }

    std::visit(
      [&conditional_arg_size_cache, &total_size](auto const& elem)
      {
        total_size +=
          Codec<std::decay_t<decltype(elem)>>::compute_encoded_size(conditional_arg_size_cache, elem);
      },
      arg);

    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    if (arg.valueless_by_exception())
    {
      Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, std::variant_npos);

      if constexpr (can_preserve_valueless_state)
      {
        DeferredFormatCodec<VariantType>::encode(
          buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, std::forward<Arg>(arg));
      }

      return;
    }

    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.index());

#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if constexpr (std::is_rvalue_reference_v<Arg&&>)
    {
      std::visit(
        [&buffer, &conditional_arg_size_cache, &conditional_arg_size_cache_index](auto&& elem)
        {
          Codec<std::decay_t<decltype(elem)>>::encode(
            buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, std::move(elem));
        },
        std::move(arg));
    }
    else
    {
      std::visit(
        [&buffer, &conditional_arg_size_cache, &conditional_arg_size_cache_index](auto const& elem)
        {
          Codec<std::decay_t<decltype(elem)>>::encode(buffer, conditional_arg_size_cache,
                                                      conditional_arg_size_cache_index, elem);
        },
        arg);
    }
#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic pop
#endif
  }

  static DecodedVariantType decode_arg(std::byte*& buffer)
  {
    size_t const active_index = Codec<size_t>::decode_arg(buffer);

    if (active_index == std::variant_npos)
    {
      if constexpr (can_preserve_valueless_state)
      {
        return DecodedVariantType{
          std::optional<DecodedVariantValueType>{DeferredFormatCodec<VariantType>::decode_arg(buffer)}};
      }
      else
      {
        return DecodedVariantType{};
      }
    }

    if (active_index >= sizeof...(Types))
    {
      QUILL_ASSERT(false,
                   "Codec<std::variant<Types...>>::decode_arg() decoded an invalid variant index");
      std::abort();
    }

    return decode_by_index(buffer, active_index, std::index_sequence_for<Types...>{});
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};
QUILL_END_EXPORT

QUILL_END_NAMESPACE

template <typename... Types, typename Char>
struct fmtquill::formatter<quill::detail::DecodedVariant<Types...>, Char>
{
private:
  fmtquill::formatter<std::variant<Types...>, Char> _underlying;

public:
  FMTQUILL_CONSTEXPR auto parse(fmtquill::parse_context<Char>& ctx) -> Char const*
  {
    return _underlying.parse(ctx);
  }

  template <typename FormatContext>
  auto format(quill::detail::DecodedVariant<Types...> const& decoded_variant, FormatContext& ctx) const
    -> decltype(ctx.out())
  {
    if (!decoded_variant.value.has_value())
    {
      return fmtquill::detail::write<Char>(ctx.out(), "variant(valueless by exception)");
    }

    return _underlying.format(*decoded_variant.value, ctx);
  }
};

#if defined(_WIN32) && defined(_MSC_VER)
  #pragma warning(pop)
#endif
