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
#include "quill/bundled/fmt/std.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
#include <variant>

QUILL_BEGIN_NAMESPACE

namespace detail
{
template <typename T>
struct DecodedVariantAlternative
{
  using type =
    std::remove_cv_t<std::remove_reference_t<decltype(Codec<T>::decode_arg(std::declval<std::byte*&>()))>>;
};

template <typename T>
using decoded_variant_alternative_t = typename DecodedVariantAlternative<T>::type;

template <size_t Index, typename VariantType, typename DecodedType>
QUILL_NODISCARD inline VariantType make_decoded_variant(DecodedType&& decoded_type)
{
  using StoredType = std::remove_reference_t<DecodedType>;

  if constexpr (std::is_move_constructible_v<StoredType>)
  {
    return VariantType{std::in_place_index<Index>, std::forward<DecodedType>(decoded_type)};
  }
  else
  {
    return VariantType{std::in_place_index<Index>, decoded_type};
  }
}

template <typename T>
struct StoredObjectCodec
{
  static constexpr bool use_memcpy =
    std::conjunction_v<std::is_trivially_copyable<T>, std::is_default_constructible<T>>;

  static size_t compute_encoded_size() noexcept
  {
    if constexpr (use_memcpy)
    {
      return sizeof(T);
    }
    else
    {
      return sizeof(T) + alignof(T) - 1;
    }
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, Arg&& arg)
  {
    if constexpr (use_memcpy)
    {
      std::memcpy(buffer, &arg, sizeof(arg));
      buffer += sizeof(arg);
    }
    else
    {
      auto* aligned_ptr = align_pointer(buffer, alignof(T));

      if constexpr (std::is_rvalue_reference_v<Arg&&> && std::is_move_constructible_v<T>)
      {
        new (static_cast<void*>(aligned_ptr)) T(std::move(arg));
      }
      else
      {
        static_assert(std::is_copy_constructible_v<T>, "T must be copy-constructible");
        new (static_cast<void*>(aligned_ptr)) T(arg);
      }

      buffer += sizeof(T) + alignof(T) - 1;
    }
  }

  static T decode(std::byte*& buffer)
  {
    if constexpr (use_memcpy)
    {
      T arg;
      std::memcpy(static_cast<void*>(&arg), buffer, sizeof(arg));
      buffer += sizeof(arg);
      return arg;
    }
    else
    {
      static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>,
                    "T must be move or copy constructible");

      auto* aligned_ptr = align_pointer(buffer, alignof(T));
      auto* tmp = std::launder(reinterpret_cast<T*>(aligned_ptr));
      buffer += sizeof(T) + alignof(T) - 1;

      if constexpr (std::is_move_constructible_v<T>)
      {
        T arg{std::move(*tmp)};
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
          tmp->~T();
        }
        return arg;
      }
      else
      {
        T arg{*tmp};
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
          tmp->~T();
        }
        return T{arg};
      }
    }
  }

private:
  static std::byte* align_pointer(void* pointer, size_t alignment) noexcept
  {
    return reinterpret_cast<std::byte*>((reinterpret_cast<uintptr_t>(pointer) + (alignment - 1ul)) &
                                        ~(alignment - 1ul));
  }
};
} // namespace detail

QUILL_BEGIN_EXPORT

template <>
struct Codec<std::monostate>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::monostate const& arg) noexcept
  {
    (void)conditional_arg_size_cache;
    (void)arg;
    return 0;
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
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
  using DecodedVariantType = std::variant<detail::decoded_variant_alternative_t<Types>...>;
  static constexpr bool can_preserve_valueless_state = std::is_same_v<DecodedVariantType, VariantType>;

  static_assert(fmtquill::detail::is_variant_formattable<DecodedVariantType, char>::value,
                "Codec<std::variant<Types...>> requires the decoded variant type to be formattable "
                "by bundled fmt.");

  template <size_t Index>
  static DecodedVariantType decode_alternative(std::byte*& buffer)
  {
    using OriginalType = std::variant_alternative_t<Index, VariantType>;
    auto decoded_value = Codec<OriginalType>::decode_arg(buffer);
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
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     VariantType const& arg) noexcept
  {
    size_t total_size{sizeof(size_t)};

    if (arg.valueless_by_exception())
    {
      if constexpr (can_preserve_valueless_state)
      {
        total_size += detail::StoredObjectCodec<VariantType>::compute_encoded_size();
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
  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg) noexcept
  {
    if (arg.valueless_by_exception())
    {
      Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, std::variant_npos);

      if constexpr (can_preserve_valueless_state)
      {
        detail::StoredObjectCodec<VariantType>::encode(buffer, std::forward<Arg>(arg));
      }

      return;
    }

    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.index());

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
  }

  static DecodedVariantType decode_arg(std::byte*& buffer)
  {
    size_t const active_index = Codec<size_t>::decode_arg(buffer);

    if (active_index == std::variant_npos)
    {
      if constexpr (can_preserve_valueless_state)
      {
        return detail::StoredObjectCodec<DecodedVariantType>::decode(buffer);
      }
      else
      {
        QUILL_ASSERT(
          false,
          "Codec<std::variant<Types...>>::decode_arg() does not support valueless_by_exception "
          "when decoded alternative types differ from the stored alternative types");
        std::abort();
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
