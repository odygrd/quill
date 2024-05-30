/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#ifndef __STDC_WANT_LIB_EXT1__
  #define __STDC_WANT_LIB_EXT1__ 1
#endif

#include "quill/bundled/fmt/core.h"

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

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <memory>
  #include <windows.h>
#endif

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

#if defined(_WIN32)
/**
 * @brief Convert a wide Unicode string to a UTF-8 encoded string.
 *
 * @param input_string_data Pointer to the wide string data.
 * @param input_string_length Length of the wide string.
 * @return The UTF-8 encoded string.
 *
 * @remarks If the input string is empty or the conversion fails, an empty string is returned.
 */
inline std::string utf8_encode(std::byte const* data, size_t wide_str_len)
{
  // Check if the input is empty
  if (wide_str_len == 0)
  {
    return std::string{};
  }

  // Create a unique_ptr to hold the buffer and one for the null terminator
  std::unique_ptr<wchar_t[]> wide_buffer{new wchar_t[wide_str_len + 1]};

  // Because we are using a std::byte* buffer and the data are coming from there we will cast
  // back the data to char* and then copy them to a wide string buffer before accessing them
  std::memcpy(wide_buffer.get(), data, wide_str_len * sizeof(wchar_t));
  wide_buffer[wide_str_len] = L'\0';

  // Calculate the size needed for the UTF-8 string
  int const size_needed = WideCharToMultiByte(
    CP_UTF8, 0, wide_buffer.get(), static_cast<int>(wide_str_len), nullptr, 0, nullptr, nullptr);

  // Check for conversion failure
  if (size_needed == 0)
  {
    return std::string{};
  }

  // Create a buffer to hold the UTF-8 string
  std::string ret_val(static_cast<size_t>(size_needed), 0);

  // Convert the wide string to UTF-8
  if (WideCharToMultiByte(CP_UTF8, 0, wide_buffer.get(), static_cast<int>(wide_str_len),
                          &ret_val[0], size_needed, nullptr, nullptr) == 0)
  {
    // conversion failure
    return std::string{};
  }

  return ret_val;
}

inline std::string utf8_encode(std::wstring_view str)
{
  return utf8_encode(reinterpret_cast<std::byte const*>(str.data()), str.size());
}
#endif

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
#if defined(_WIN32)
    else if constexpr (std::disjunction_v<std::is_same<Arg, wchar_t*>, std::is_same<Arg, wchar_t const*>,
                                          std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>)
    {
      // Calculate the size of the string in bytes
      size_t len;

      if constexpr (std::disjunction_v<std::is_same<Arg, wchar_t*>, std::is_same<Arg, wchar_t const*>>)
      {
        len = wcslen(arg);
      }
      else
      {
        len = arg.size();
      }

      conditional_arg_size_cache.push_back(len);

      // also include the size of the string in the buffer as a separate variable
      // we can retrieve it when we decode. We do not store the null terminator in the buffer
      return static_cast<size_t>(sizeof(size_t) + (len * sizeof(wchar_t)));
    }
#endif
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
#if defined(_WIN32)
    else if constexpr (std::disjunction_v<std::is_same<Arg, wchar_t*>, std::is_same<Arg, wchar_t const*>,
                                          std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>)
    {
      // The wide string size in bytes
      size_t const len = conditional_arg_size_cache[conditional_arg_size_cache_index++];
      std::memcpy(buffer, &len, sizeof(len));
      buffer += sizeof(len);

      if (len != 0)
      {
        // copy the string, no need to zero terminate it as we got the length and e.g a wstring_view
        // might not always be zero terminated
        size_t const size_in_bytes = len * sizeof(wchar_t);

        if constexpr (std::disjunction_v<std::is_same<Arg, wchar_t*>, std::is_same<Arg, wchar_t const*>>)
        {
          std::memcpy(buffer, arg, size_in_bytes);
        }
        else
        {
          std::memcpy(buffer, arg.data(), size_in_bytes);
        }

        buffer += size_in_bytes;
      }
    }
#endif
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
        // pass the std::string_view to args_store to avoid the dynamic allocation
        args_store->push_back(std::string_view{str, len});
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
        args_store->push_back(v);
      }

      return v;
    }
#if defined(_WIN32)
    else if constexpr (std::disjunction_v<std::is_same<Arg, wchar_t*>, std::is_same<Arg, wchar_t const*>,
                                          std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>)
    {
      // we first need to retrieve the length
      size_t len;
      std::memcpy(&len, buffer, sizeof(len));
      buffer += sizeof(len);

      std::wstring_view wstr{reinterpret_cast<wchar_t const*>(buffer), len};

      if (args_store)
      {
        std::string str = utf8_encode(buffer, len);
        args_store->push_back(static_cast<std::string&&>(str));
      }

      size_t size_bytes = len * sizeof(wchar_t);
      buffer += size_bytes;
      return wstr;
    }
#endif
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
