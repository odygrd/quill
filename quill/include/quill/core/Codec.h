/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#ifndef __STDC_WANT_LIB_EXT1__
  #define __STDC_WANT_LIB_EXT1__ 1
#endif

#include "quill/bundled/fmt/base.h"

#include "quill/core/Attributes.h"
#include "quill/core/DynamicFormatArgStore.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace quill::detail
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

constexpr auto strnlen =
#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
  ::strnlen_s
#else
  ::strnlen
#endif
  ;

/** typename = void for specializations with enable_if **/
template <typename Arg, typename = void>
struct ArgSizeCalculator
{
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT static size_t calculate(QUILL_MAYBE_UNUSED std::vector<size_t>& conditional_arg_size_cache,
                                                              QUILL_MAYBE_UNUSED Arg const& arg) noexcept
  {
    if constexpr (std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>)
    {
      return sizeof(Arg);
    }
    else if constexpr (std::conjunction_v<std::is_array<Arg>, std::is_same<remove_cvref_t<std::remove_extent_t<Arg>>, char>>)
    {
      size_t constexpr N = std::extent_v<Arg>;
      conditional_arg_size_cache.push_back(static_cast<size_t>(strnlen(arg, N) + 1u));
      return conditional_arg_size_cache.back();
    }
    else if constexpr (std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>>)
    {
      // include one extra for the zero termination
      conditional_arg_size_cache.push_back(static_cast<size_t>(strlen(arg) + 1u));
      return conditional_arg_size_cache.back();
    }
    else if constexpr (std::disjunction_v<std::is_same<Arg, std::string>, std::is_same<Arg, std::string_view>>)
    {
      // for std::string we also need to store the size in order to correctly retrieve it
      // the reason for this is that if we create e.g:
      // std::string msg = fmtquill::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
      // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
      return sizeof(size_t) + arg.length();
    }
    else
    {
      static_assert(always_false_v<Arg>, "Unsupported type");
    }
  }
};

/**
 * @brief Calculates the total size required to encode the provided arguments

 * @param conditional_arg_size_cache Storage to avoid repeating calculations eg. cache strlen
 * @param args The arguments to be encoded.
 * @return The total size required to encode the arguments.
 */
template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t calculate_args_size_and_populate_string_lengths(
  QUILL_MAYBE_UNUSED std::vector<size_t>& conditional_arg_size_cache, Args const&... args) noexcept
{
  // Do not use fold expression with '+ ...' as we need a guaranteed sequence for the args here
  size_t total_sum{0};
  ((total_sum += ArgSizeCalculator<remove_cvref_t<Args>>::calculate(conditional_arg_size_cache, args)), ...);
  return total_sum;
}

/** typename = void for specializations with enable_if **/
template <typename Arg, typename = void>
struct Encoder
{
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*& buffer,
                                         QUILL_MAYBE_UNUSED std::vector<size_t> const& conditional_arg_size_cache,
                                         QUILL_MAYBE_UNUSED uint32_t& conditional_arg_size_cache_index,
                                         Arg const& arg) noexcept
  {
    if constexpr (std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>)
    {
      std::memcpy(buffer, &arg, sizeof(Arg));
      buffer += sizeof(Arg);
    }
    else if constexpr (std::conjunction_v<std::is_array<Arg>, std::is_same<remove_cvref_t<std::remove_extent_t<Arg>>, char>>)
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
    else if constexpr (std::disjunction_v<std::is_same<Arg, std::string>, std::is_same<Arg, std::string_view>>)
    {
      // for std::string we store the size first, in order to correctly retrieve it
      // Copy the length first and then the actual string
      size_t const len = arg.length();
      std::memcpy(buffer, &len, sizeof(len));

      if (len != 0)
      {
        // copy the string, no need to zero terminate it as we got the length
        std::memcpy(buffer + sizeof(len), arg.data(), arg.length());
      }

      buffer += sizeof(len) + len;
    }
    else
    {
      static_assert(always_false_v<Arg>, "Unsupported type");
    }
  }
};

/**
 * @brief Encoders multiple arguments into a buffer.
 * @param buffer Pointer to the buffer for encoding.
 * @param conditional_arg_size_cache Storage to avoid repeating calculations eg. cache strlen
 * @param args The arguments to be encoded.
 */
template <typename... Args>
QUILL_ATTRIBUTE_HOT void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                                Args const&... args) noexcept
{
  QUILL_MAYBE_UNUSED uint32_t conditional_arg_size_cache_index{0};
  (Encoder<remove_cvref_t<Args>>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, args),
   ...);
}

/** typename = void for specializations with enable_if **/
template <typename Arg, typename = void>
struct Decoder
{
  static auto decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    if constexpr (std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>)
    {
      Arg arg;
      std::memcpy(&arg, buffer, sizeof(Arg));
      buffer += sizeof(Arg);

      if (args_store)
      {
        args_store->push_back(arg);
      }

      return arg;
    }
    else if constexpr (std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>,
                                          std::conjunction<std::is_array<Arg>, std::is_same<remove_cvref_t<std::remove_extent_t<Arg>>, char>>>)
    {
      // c strings or char array
      char const* str = reinterpret_cast<char const*>(buffer);
      size_t const len = strlen(str);
      buffer += len + 1; // for c_strings we add +1 to the length as we also want to copy the null terminated char

      if (args_store)
      {
        // pass the string_view to args_store to avoid the dynamic allocation
        // we pass fmtquill::string_view since fmt/base.h includes a formatter for that type.
        // for std::string_view we would need fmt/format.h
        args_store->push_back(fmtquill::string_view{str, len});
      }

      return str;
    }
    else if constexpr (std::disjunction_v<std::is_same<Arg, std::string>, std::is_same<Arg, std::string_view>>)
    {
      // for std::string we first need to retrieve the length
      size_t len;
      std::memcpy(&len, buffer, sizeof(len));

      // retrieve the rest of the string
      char const* str = reinterpret_cast<char const*>(buffer + sizeof(size_t));
      std::string_view v{str, len};
      buffer += sizeof(len) + v.length();

      if (args_store)
      {
        // pass the string_view to args_store to avoid the dynamic allocation
        // we pass fmtquill::string_view since fmt/base.h includes a formatter for that type.
        // for std::string_view we would need fmt/format.h
        args_store->push_back(fmtquill::string_view{v.data(), v.size()});
      }

      return v;
    }
    else
    {
      static_assert(always_false_v<Arg>, "Unsupported type");
    }
  }
};

template <typename... Args>
void decode(std::byte*& buffer, QUILL_MAYBE_UNUSED DynamicFormatArgStore* args_store) noexcept
{
  (Decoder<Args>::decode(buffer, args_store), ...);
}

/**
 * Decoder functions
 */
using FormatArgsDecoder = void (*)(std::byte*& data, DynamicFormatArgStore& args_store);

template <typename... Args>
void decode_and_populate_format_args(std::byte*& buffer, DynamicFormatArgStore& args_store)
{
  args_store.clear();
  decode<Args...>(buffer, &args_store);
}

/** Codec helpers for user defined types convenience **/
/***/
template <typename... TMembers>
size_t calculate_total_size(std::vector<size_t>& conditional_arg_size_cache, TMembers const&... members)
{
  size_t total_size{0};
  ((total_size += ArgSizeCalculator<remove_cvref_t<TMembers>>::calculate(conditional_arg_size_cache, members)), ...);
  return total_size;
}

/***/
template <typename... TMembers>
void encode_members(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                    uint32_t& conditional_arg_size_cache_index, TMembers const&... members)
{
  ((Encoder<remove_cvref_t<TMembers>>::encode(buffer, conditional_arg_size_cache,
                                              conditional_arg_size_cache_index, members)),
   ...);
}

/***/
template <typename T, typename... TMembers>
void decode_and_assign_members(std::byte*& buffer, DynamicFormatArgStore* args_store, T& arg, TMembers&... members)
{
  ((members = Decoder<remove_cvref_t<TMembers>>::decode(buffer, nullptr)), ...);

  if (args_store)
  {
    args_store->push_back(arg);
  }
}
} // namespace quill::detail
