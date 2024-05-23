#pragma once

// Always required
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"

// To serialise the std::array member of User you need Array.h otherwise you don't need to include this
#include "quill/std/Array.h"

#include "user.h"

#include <utility> // for declval only required if you do the decoding manualy and use declval

/***/
template <>
struct fmtquill::formatter<User>
{
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(::User const& user, FormatContext& ctx) const
  {
    return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
                               user.name, user.surname, user.age, user.favorite_colors);
  }
};

/***/
template <>
struct quill::detail::ArgSizeCalculator<User>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, ::User const& user) noexcept
  {
    // pass as arguments the class members you want to serialize
    return calculate_total_size(conditional_arg_size_cache, user.name, user.surname, user.age, user.favorite_colors);
  }
};

/***/
template <>
struct quill::detail::Encoder<User>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, ::User const& user) noexcept
  {
    // You must encode the same members and in the same order as in the ArgSizeCalculator::calculate
    encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, user.name,
                   user.surname, user.age, user.favorite_colors);
  }
};

/***/
template <>
struct quill::detail::Decoder<User>
{
  static ::User decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    // You must decode the same members and in the same order as in the Encoder::encode
    ::User user;
    decode_and_assign_members(buffer, args_store, user, user.name, user.surname, user.age, user.favorite_colors);
    return user;

    // note:
    // If the object is not default constructible you have to do it manually without
    // decode_members helper

    // auto name = Decoder<decltype(std::declval<::User>().name)>::decode(buffer, nullptr);
    // auto surname = Decoder<decltype(std::declval<::User>().surname)>::decode(buffer, nullptr);
    // auto age = Decoder<decltype(std::declval<::User>().age)>::decode(buffer, nullptr);
    // auto favorite_colors = Decoder<decltype(std::declval<::User>().favorite_colors)>::decode(buffer, nullptr);

    // ::User user{name, surname, age, favorite_colors};

    // if (args_store)
    // {
    //  args_store->push_back(user);
    // }

    // return user;
  }
};