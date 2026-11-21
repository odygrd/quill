/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/format.h"
#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <string_view>
  #include <type_traits>
#endif

QUILL_BEGIN_NAMESPACE

/**
 * @brief Provides codec functionality for serializing complex user-defined types into strings.
 *
 * This codec is particularly useful for logging user-defined types, it converts an object into a
 * string representation using the `fmt::formatter`on the hot path.
 *
 * When using this codec, a `fmt::formatter` specialization must exist for the user-defined type.
 *
 * It eliminates the need to explicitly write more verbose code, such as:
 *
 * ```cpp
 * LOG_INFO(l, "{}", fmtquill::format("{}", obj));
 * ```
 *
 * Instead, you can directly write:
 *
 * ```cpp
 * LOG_INFO(logger, "{}", obj);
 * ```
 *
 * \code{.cpp}
 * class User
 * {
 * public:
 *   User(std::string name, std::string surname, uint32_t age)
 *     : name(std::move(name)), surname(std::move(surname)), age(age)
 *   {
 *     favorite_colors.push_back("red");
 *     favorite_colors.push_back("blue");
 *     favorite_colors.push_back("green");
 *   };
 *
 *   std::string name;
 *   std::string surname;
 *   uint32_t age;
 *   std::vector<std::string> favorite_colors;
 * };
 *
 * template <>
 * struct fmtquill::formatter<User>
 * {
 *   constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
 *
 *   auto format(::User const& user, format_context& ctx) const
 *   {
 *     return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
 *                                user.name, user.surname, user.age, user.favorite_colors);
 *   }
 * };
 *
 * template <>
 * struct quill::Codec<User> : quill::DirectFormatCodec<User>
 * {
 * };
 *
 * int main()
 * {
 *   // ... init code
 *   User user_1{"Super", "User", 1};
 *   LOG_INFO(logger, "User is [{}]", user_1);
 * }
 * \endcode
 */

QUILL_BEGIN_EXPORT

/**
 * Formats the argument once on the calling thread, while sizing the record, and copies the
 * formatted bytes into the queue. The logged text is therefore one consistent snapshot even if the
 * formatter reads state that changes concurrently, and a throwing formatter fails the log call
 * before anything is queued. The thread-local size cache retains its capacity for later records
 * until the thread context is destroyed.
 */
template <typename T>
struct DirectFormatCodec
{
  static size_t compute_encoded_size(quill::SizeCacheVector& conditional_arg_size_cache, T const& arg)
  {
    fmtquill::memory_buffer formatted;

    if constexpr (fmtquill::detail::is_view<T>::value)
    {
      // fmt disallows formatting view types as lvalues; formatting a temporary copy keeps
      // DirectFormatCodec usable for cheap fmt views such as fmtquill::join_view.
      fmtquill::format_to(fmtquill::appender{formatted}, "{}", T{arg});
    }
    else
    {
      fmtquill::format_to(fmtquill::appender{formatted}, "{}", arg);
    }

    return detail::cache_string(conditional_arg_size_cache,
                                std::string_view{formatted.data(), formatted.size()});
  }

  static void encode(std::byte*& buffer, quill::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, T const&) noexcept
  {
    detail::encode_cached_string(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index);
  }

  static std::string_view decode_arg(std::byte*& buffer)
  {
    return quill::Codec<std::string>::decode_arg(buffer);
  }

  static void decode_and_store_arg(std::byte*& buffer, quill::DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

// fmt views, e.g. fmtquill::join_view, are provided by optional fmt headers. Detect them
// through fmt's view trait instead of naming each view type or including those headers here.
template <typename T>
struct Codec<T, std::enable_if_t<fmtquill::detail::is_view<T>::value>> : DirectFormatCodec<T>
{
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
