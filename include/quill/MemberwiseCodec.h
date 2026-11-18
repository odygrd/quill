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

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <type_traits>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * A codec that copies the listed data members to the backend, which rebuilds the object from them
 * and formats it with its fmtquill::formatter. Members are listed once, in one place, so sizing,
 * encoding and decoding always handle the same members in the same order.
 *
 * Use it when an object is not safe or cheap to copy whole, but the members needed to format it
 * are. T must be default constructible, and each member's type must have a Codec, e.g. a quill/std
 * header for STL members, whose decoded value can be assigned to the member. Containers of
 * strings decode to containers of std::string_view and need a hand-written codec.
 *
 * @code
 * struct Order
 * {
 *   std::string symbol;
 *   double price;
 *   uint32_t quantity;
 * };
 *
 * template <>
 * struct quill::Codec<Order> : quill::MemberwiseCodec<Order, &Order::symbol, &Order::price, &Order::quantity>
 * {
 * };
 * @endcode
 */
template <typename T, auto... Members>
struct MemberwiseCodec
{
  static_assert(sizeof...(Members) > 0, "MemberwiseCodec requires at least one member");
  static_assert((std::is_member_object_pointer_v<decltype(Members)> && ...),
                "MemberwiseCodec expects pointers to data members, e.g. &T::member");
  static_assert(std::is_default_constructible_v<T>,
                "MemberwiseCodec rebuilds T on the backend, so T must be default constructible");

  static size_t compute_encoded_size(SizeCacheVector& conditional_arg_size_cache, T const& value)
  {
    return compute_total_encoded_size(conditional_arg_size_cache, (value.*Members)...);
  }

  static void encode(std::byte*& buffer, SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, T const& value)
  {
    encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, (value.*Members)...);
  }

  static T decode_arg(std::byte*& buffer)
  {
    // Value-initialise so members that are not listed are not left indeterminate
    T value{};
    decode_members(buffer, value, (value.*Members)...);
    return value;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
