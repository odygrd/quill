#pragma once

// Always required
#include "quill/bundled/fmt/format.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"

// To serialise the std::array member of User you need Array.h otherwise you don't need to include this
#include "quill/std/Array.h"

#include "user.h"

#include <utility> // for declval only required if you do the decoding manualy and use declval

/***/
template <>
struct fmtquill::formatter<User>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(::User const& user, format_context& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
                               user.name, user.surname, user.age, user.favorite_colors);
  }
};

/***/
template <>
struct quill::Codec<User>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache, ::User const& user) noexcept
  {
    // pass as arguments the class members you want to serialize
    return compute_total_encoded_size(conditional_arg_size_cache, user.name, user.surname, user.age,
                                      user.favorite_colors);
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, ::User const& user) noexcept
  {
    // You must encode the same members and in the same order as in compute_total_encoded_size
    encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, user.name,
                   user.surname, user.age, user.favorite_colors);
  }

  static ::User decode_arg(std::byte*& buffer)
  {
    // You must decode the same members and in the same order as in encode
    ::User user;
    decode_members(buffer, user, user.name, user.surname, user.age, user.favorite_colors);
    return user;

    // note:
    // If the object is not default constructible you can also do it manually without
    // decode_members helper

    // auto name = Codec<decltype(std::declval<::User>().name)>::decode_arg(buffer);
    // auto surname = Codec<decltype(std::declval<::User>().surname)>::decode_arg(buffer);
    // auto age = Codec<decltype(std::declval<::User>().age)>::decode_arg(buffer);
    // auto favorite_colors = Codec<decltype(std::declval<::User>().favorite_colors)>::decode_arg(buffer);

    // ::User user{name, surname, age, favorite_colors};

    // return user;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};