/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/base.h"

#include "quill/core/Attributes.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * C++14 implementation of C++20's remove_cvref
 */
template <class T>
struct remove_cvref
{
  typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};

template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <class>
constexpr bool always_false_v = false;

template <typename Arg>
void codec_not_found_for_type()
{
  static_assert(
    always_false_v<Arg>,
    "\n"
    "+------------------------------------------------------------------------------+\n"
    "|                       Missing Codec for Type 'Arg'                           |\n"
    "+------------------------------------------------------------------------------+\n"
    "\n"
    "Error: A codec for the specified type 'Arg' is not available.\n"
    "\n"
    "Possible solutions:\n"
    "1. If Arg is an STL type:\n"
    "   - Ensure you have included the necessary headers for the specific STL type you are using "
    "from the quill/std folder.\n"
    "\n"
    "2. If Arg is a user-defined type:\n"
    "   - Define a custom Codec for your type.\n"
    "   - Consider passing the value as a string instead.\n"
    "\n"
    "Note: The specific type of 'Arg' can be found in the compiler error message.\n"
    "      Look for the instantiation of 'codec_not_found_for_type<Arg>' in the error output.\n"
    "      The compiler should indicate what type 'Arg' represents in this instance.\n"
    "\n"
    "For more information see https://quillcpp.readthedocs.io/en/latest/cheat_sheet.html\n");
}

QUILL_NODISCARD inline size_t safe_strnlen(char const* str, size_t maxlen) noexcept
{
  char const* end = static_cast<char const*>(std::memchr(str, '\0', maxlen));
  return end ? static_cast<size_t>(end - str) : maxlen;
}

/** std string detection, ignoring the Allocator type **/
template <typename T>
struct is_std_string : std::false_type
{
};

template <typename Allocator>
struct is_std_string<std::basic_string<char, std::char_traits<char>, Allocator>> : std::true_type
{
};
} // namespace detail

/** typename = void for specializations with enable_if **/
template <typename Arg, typename = void>
struct Codec
{
  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT static size_t compute_encoded_size(
    QUILL_MAYBE_UNUSED detail::SizeCacheVector& conditional_arg_size_cache, QUILL_MAYBE_UNUSED Arg const& arg) noexcept
  {
    if constexpr (std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>, std::is_same<Arg, void const*>>)
    {
      return sizeof(Arg);
    }
    else if constexpr (std::conjunction_v<std::is_array<Arg>, std::is_same<detail::remove_cvref_t<std::remove_extent_t<Arg>>, char>>)
    {
      size_t constexpr N = std::extent_v<Arg>;
      assert(((detail::safe_strnlen(arg, N) + 1u) <= std::numeric_limits<uint32_t>::max()) &&
             "len is outside the supported range");
      return conditional_arg_size_cache.push_back(static_cast<uint32_t>(detail::safe_strnlen(arg, N) + 1u));
    }
    else if constexpr (std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>>)
    {
      // include one extra for the zero termination
      assert(((strlen(arg) + 1u) <= std::numeric_limits<uint32_t>::max()) &&
             "len is outside the supported range");
      return conditional_arg_size_cache.push_back(static_cast<uint32_t>(strlen(arg) + 1u));
    }
    else if constexpr (std::disjunction_v<detail::is_std_string<Arg>, std::is_same<Arg, std::string_view>>)
    {
      // for std::string we also need to store the size in order to correctly retrieve it
      // the reason for this is that if we create e.g:
      // std::string msg = fmtquill::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
      // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
      return sizeof(size_t) + arg.length();
    }
    else
    {
      detail::codec_not_found_for_type<Arg>();
      return 0;
    }
  }

  /***/
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*& buffer,
                                         QUILL_MAYBE_UNUSED detail::SizeCacheVector const& conditional_arg_size_cache,
                                         QUILL_MAYBE_UNUSED uint32_t& conditional_arg_size_cache_index,
                                         Arg const& arg) noexcept
  {
    if constexpr (std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>, std::is_same<Arg, void const*>>)
    {
      std::memcpy(buffer, &arg, sizeof(Arg));
      buffer += sizeof(Arg);
    }
    else if constexpr (std::conjunction_v<std::is_array<Arg>, std::is_same<detail::remove_cvref_t<std::remove_extent_t<Arg>>, char>>)
    {
      size_t constexpr N = std::extent_v<Arg>;
      size_t const len = conditional_arg_size_cache[conditional_arg_size_cache_index++];

      if (QUILL_UNLIKELY(len > N))
      {
        // no '\0' in c array
        assert(len == N + 1);
        std::memcpy(buffer, arg, N);
        buffer[len - 1] = std::byte{'\0'};
      }
      else
      {
        std::memcpy(buffer, arg, len);
      }

      buffer += len;
    }
    else if constexpr (std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>>)
    {
      // null terminator is included in the len for c style strings
      size_t const len = conditional_arg_size_cache[conditional_arg_size_cache_index++];
      std::memcpy(buffer, arg, len);
      buffer += len;
    }
    else if constexpr (std::disjunction_v<detail::is_std_string<Arg>, std::is_same<Arg, std::string_view>>)
    {
      // for std::string we store the size first, in order to correctly retrieve it
      // Copy the length first and then the actual string
      size_t const len = arg.length();
      std::memcpy(buffer, &len, sizeof(len));
      buffer += sizeof(len);
      std::memcpy(buffer, arg.data(), len);
      buffer += len;
    }
    else
    {
      detail::codec_not_found_for_type<Arg>();
    }
  }

  // We use two separate functions, decode_arg and decode_and_store_arg, because there are
  // scenarios where we need to decode an argument without storing it in args_store, such as when
  // dealing with nested types. Storing the argument requires a fmtquill formatter, so having
  // two distinct functions allows us to avoid this requirement in cases where only decoding is needed.

  /***/
  static auto decode_arg(std::byte*& buffer)
  {
    if constexpr (std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>, std::is_same<Arg, void const*>>)
    {
      Arg arg;
      std::memcpy(&arg, buffer, sizeof(Arg));
      buffer += sizeof(Arg);
      return arg;
    }
    else if constexpr (std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>,
                                          std::conjunction<std::is_array<Arg>, std::is_same<detail::remove_cvref_t<std::remove_extent_t<Arg>>, char>>>)
    {
      // c strings or char array
      char const* arg = reinterpret_cast<char const*>(buffer);
      buffer += strlen(arg) + 1; // for c_strings we add +1 to the length as we also want to copy the null terminated char
      return arg;
    }
    else if constexpr (std::disjunction_v<detail::is_std_string<Arg>, std::is_same<Arg, std::string_view>>)
    {
      // for std::string we first need to retrieve the length
      size_t len;
      std::memcpy(&len, buffer, sizeof(len));
      buffer += sizeof(len);
      std::string_view const arg = std::string_view{reinterpret_cast<char const*>(buffer), len};
      buffer += len;
      return arg;
    }
    else
    {
      detail::codec_not_found_for_type<Arg>();
      return 0;
    }
  }

  /***/
  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    if constexpr (std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>, std::is_same<Arg, void const*>>)
    {
      args_store->push_back(decode_arg(buffer));
    }
    else if constexpr (std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>,
                                          std::conjunction<std::is_array<Arg>, std::is_same<detail::remove_cvref_t<std::remove_extent_t<Arg>>, char>>>)
    {
      // c strings or char array
      char const* arg = decode_arg(buffer);

      // pass the string_view to args_store to avoid the dynamic allocation
      // we pass fmtquill::string_view since fmt/base.h includes a formatter for that type.
      // for std::string_view we would need fmt/format.h
      args_store->push_back(fmtquill::string_view{arg, strlen(arg)});
    }
    else if constexpr (std::disjunction_v<detail::is_std_string<Arg>, std::is_same<Arg, std::string_view>>)
    {
      std::string_view arg = decode_arg(buffer);

      // pass the string_view to args_store to avoid the dynamic allocation
      // we pass fmtquill::string_view since fmt/base.h includes a formatter for that type.
      // for std::string_view we would need fmt/format.h
      args_store->push_back(fmtquill::string_view{arg.data(), arg.size()});
    }
    else
    {
      detail::codec_not_found_for_type<Arg>();
    }
  }
};

namespace detail
{
/**
 * @brief Calculates the total size required to encode the provided arguments

 * @param conditional_arg_size_cache Storage to avoid repeating calculations eg. cache strlen
 * @param args The arguments to be encoded.
 * @return The total size required to encode the arguments.
 */
template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t compute_encoded_size_and_cache_string_lengths(
  QUILL_MAYBE_UNUSED detail::SizeCacheVector& conditional_arg_size_cache, Args const&... args) noexcept
{
  // Do not use fold expression with '+ ...' as we need a guaranteed sequence for the args here
  size_t total_sum{0};
  ((total_sum += Codec<detail::remove_cvref_t<Args>>::compute_encoded_size(conditional_arg_size_cache, args)), ...);
  return total_sum;
}

/**
 * @brief Encodes multiple arguments into a buffer.
 * @param buffer Pointer to the buffer for encoding.
 * @param conditional_arg_size_cache Storage to avoid repeating calculations eg. cache strlen
 * @param args The arguments to be encoded.
 */
template <typename... Args>
QUILL_ATTRIBUTE_HOT void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                                Args const&... args) noexcept
{
  QUILL_MAYBE_UNUSED uint32_t conditional_arg_size_cache_index{0};
  (Codec<detail::remove_cvref_t<Args>>::encode(buffer, conditional_arg_size_cache,
                                               conditional_arg_size_cache_index, args),
   ...);
}

template <typename... Args>
void decode_and_store_arg(std::byte*& buffer, QUILL_MAYBE_UNUSED DynamicFormatArgStore* args_store)
{
  (Codec<Args>::decode_and_store_arg(buffer, args_store), ...);
}

/**
 * Decode functions
 */
using FormatArgsDecoder = void (*)(std::byte*& data, DynamicFormatArgStore& args_store);

template <typename... Args>
void decode_and_store_args(std::byte*& buffer, DynamicFormatArgStore& args_store)
{
  args_store.clear();
  decode_and_store_arg<Args...>(buffer, &args_store);
}
} // namespace detail

/** Codec helpers for user defined types convenience **/
/***/
template <typename... TMembers>
size_t compute_total_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache, TMembers const&... members)
{
  size_t total_size{0};
  ((total_size += Codec<detail::remove_cvref_t<TMembers>>::compute_encoded_size(conditional_arg_size_cache, members)),
   ...);
  return total_size;
}

/***/
template <typename... TMembers>
void encode_members(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                    uint32_t& conditional_arg_size_cache_index, TMembers const&... members)
{
  ((Codec<detail::remove_cvref_t<TMembers>>::encode(buffer, conditional_arg_size_cache,
                                                    conditional_arg_size_cache_index, members)),
   ...);
}

/***/
template <typename T, typename... TMembers>
void decode_members(std::byte*& buffer, T&, TMembers&... members)
{
  // T& arg is not used but if we remove it, it will crash all users who are passing the extra argument without a compile time error
  ((members = Codec<detail::remove_cvref_t<TMembers>>::decode_arg(buffer)), ...);
}

QUILL_END_NAMESPACE