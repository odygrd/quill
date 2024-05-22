/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#ifndef __STDC_WANT_LIB_EXT1__
  #define __STDC_WANT_LIB_EXT1__ 1
#endif

#include "quill/bundled/fmt/args.h"
#include "quill/bundled/fmt/core.h"
#include "quill/core/Attributes.h"

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
  static size_t calculate(std::vector<size_t>&, Arg const&) noexcept
  {
    static_assert(always_false_v<Arg>, "Unsupported type");
    return 0;
  }
};

/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>>>
{
  QUILL_ATTRIBUTE_HOT static size_t calculate(std::vector<size_t>&, Arg) noexcept
  {
    return static_cast<size_t>(sizeof(Arg));
  }
};

/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>>>>
{
  QUILL_ATTRIBUTE_HOT static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, char const* arg) noexcept
  {
    // include one extra for the zero termination
    conditional_arg_size_cache.push_back(static_cast<size_t>(strlen(arg) + 1u));
    return conditional_arg_size_cache.back();
  }
};

/***/
template <size_t N>
struct ArgSizeCalculator<char[N]>
{
  QUILL_ATTRIBUTE_HOT static size_t calculate(std::vector<size_t>& conditional_arg_size_cache,
                                              char const (&arg)[N]) noexcept
  {
    conditional_arg_size_cache.push_back(static_cast<size_t>(strnlen(arg, N) + 1u));
    return conditional_arg_size_cache.back();
  }
};

/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::string>, std::is_same<Arg, std::string_view>>>>
{
  QUILL_ATTRIBUTE_HOT static size_t calculate(std::vector<size_t>&, Arg const& arg) noexcept
  {
    // for std::string we also need to store the size in order to correctly retrieve it
    // the reason for this is that if we create e.g:
    // std::string msg = fmtquill::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
    // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
    return static_cast<size_t>(sizeof(size_t) + arg.length());
  }
};

#if defined(_WIN32)
/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, wchar_t*>, std::is_same<Arg, wchar_t const*>>>>
{
  QUILL_ATTRIBUTE_HOT static size_t calculate(std::vector<size_t>& conditional_arg_size_cache,
                                              wchar_t const* arg) noexcept
  {
    // Calculate the size of the string in bytes
    size_t const len = wcslen(arg);
    conditional_arg_size_cache.push_back(len);

    // to be safe we also store the size of the string in the buffer as a separate variable
    // we can retrieve it when we decode. We also store the null terminator in the buffer to
    // be able to return the value as wchar_t*
    return static_cast<size_t>(sizeof(size_t) + ((len + 1) * sizeof(wchar_t)));
  }
};

/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>>>
{
  QUILL_ATTRIBUTE_HOT static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, Arg const& arg) noexcept
  {
    // Calculate the size of the string in bytes
    size_t const len = arg.size();
    conditional_arg_size_cache.push_back(len);

    // to be safe we also store the size of the string in the buffer as a separate variable
    // we can retrieve it when we decode. We do not store the null terminator in the buffer
    return static_cast<size_t>(sizeof(size_t) + (len * sizeof(wchar_t)));
  }
};
#endif

/**
 * @brief Calculates the total size required to encode the provided arguments and populates the c_style_string_lengths array.

 * @param c_style_string_lengths Array to store the c_style_string_lengths of C-style strings and char arrays.
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
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*&, std::vector<size_t> const&, uint32_t&, Arg const&) noexcept
  {
    static_assert(always_false_v<Arg>, "Unsupported type");
  }
};

/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>>>
{
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*& buffer, std::vector<size_t> const&, uint32_t&, Arg arg) noexcept
  {
    std::memcpy(buffer, &arg, sizeof(arg));
    buffer += sizeof(arg);
  }
};

/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>>>>
{
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                                         uint32_t& conditional_arg_size_cache_index, char const* arg) noexcept
  {
    // null terminator is included in the len for c style strings
    size_t const len = conditional_arg_size_cache[conditional_arg_size_cache_index++];
    std::memcpy(buffer, arg, len);
    buffer += len;
  }
};

/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::string>, std::is_same<Arg, std::string_view>>>>
{
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*& buffer, std::vector<size_t> const&, uint32_t&, Arg const& arg) noexcept
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
};

/***/
template <size_t N>
struct Encoder<char[N]>
{
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                                         uint32_t& conditional_arg_size_cache_index,
                                             char const (&arg)[N]) noexcept
  {
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
};

#if defined(_WIN32)
/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, wchar_t*>, std::is_same<Arg, wchar_t const*>>>>
{
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                                         uint32_t& conditional_arg_size_cache_index, wchar_t const* arg) noexcept
  {
    // The wide string size in bytes
    size_t const len = conditional_arg_size_cache[conditional_arg_size_cache_index++];
    std::memcpy(buffer, &len, sizeof(len));
    buffer += sizeof(len);

    // copy the string including the null terminator
    size_t const size_in_bytes = (len + 1) * sizeof(wchar_t);
    std::memcpy(buffer, arg, size_in_bytes);
    buffer += size_in_bytes;
  }
};

/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>>>
{
  QUILL_ATTRIBUTE_HOT static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                                         uint32_t& conditional_arg_size_cache_index, Arg const& arg) noexcept
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
      std::memcpy(buffer, arg.data(), size_in_bytes);
      buffer += size_in_bytes;
    }
  }
};
#endif

/**
 * @brief Encoders multiple arguments into a buffer.
 * @param buffer Pointer to the buffer for encoding.
 * @param c_style_string_lengths Array storing the c_style_string_lengths of C-style strings and char arrays.
 * @param args The arguments to be encoded.
 */
template <typename... Args>
QUILL_ATTRIBUTE_HOT void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                                Args const&... args) noexcept
{
  QUILL_MAYBE_UNUSED uint32_t conditional_arg_size_cache_index{0};
  (Encoder<Args>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, args), ...);
}

/** typename = void for specializations with enable_if **/
template <typename Arg, typename = void>
struct Decoder
{
  static void decode(std::byte*&, fmtquill::dynamic_format_arg_store<fmtquill::format_context>*)
  {
    static_assert(always_false_v<Arg>, "Unsupported type");
  }
};

/***/
template <typename Arg>
struct Decoder<Arg, std::enable_if_t<std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>>>
{
  static Arg decode(std::byte*& buffer, fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
  {
    Arg arg;
    std::memcpy(&arg, buffer, sizeof(arg));
    buffer += sizeof(Arg);

    if (args_store)
    {
      args_store->push_back(arg);
    }

    return arg;
  }
};

/***/
template <typename Arg>
struct Decoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>>>>
{
  static char const* decode(std::byte*& buffer,
                            fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
  {
    char const* str = reinterpret_cast<char const*>(buffer);
    buffer += strlen(str) + 1; // for c_strings we add +1 to the length as we also want to copy the null terminated char

    if (args_store)
    {
      args_store->push_back(str);
    }

    return str;
  }
};

/***/
template <typename Arg>
struct Decoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::string>, std::is_same<Arg, std::string_view>>>>
{
  static std::string_view decode(std::byte*& buffer,
                                 fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
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
};

/***/
template <size_t N>
struct Decoder<char[N]>
{
  static char const* decode(std::byte*& buffer,
                            fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
  {
    char const* str = reinterpret_cast<char const*>(buffer);
    buffer += strlen(str) + 1; // for c_strings we add +1 to the length as we also want to copy the null terminated char

    if (args_store)
    {
      args_store->push_back(str);
    }

    return str;
  }
};

#if defined(_WIN32)
/***/
template <typename Arg>
struct Decoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, wchar_t*>, std::is_same<Arg, wchar_t const*>>>>
{
  static std::wstring_view decode(std::byte*& buffer,
                                  fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
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

    // For wchar_t* we also copy the null terminator
    size_t const size_bytes = (len + 1) * sizeof(wchar_t);
    buffer += size_bytes;
    return wstr;
  }
};

/***/
template <typename Arg>
struct Decoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>>>
{
  static std::wstring_view decode(std::byte*& buffer,
                                  fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
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

    size_t const size_bytes = len * sizeof(wchar_t);
    buffer += size_bytes;
    return wstr;
  }
};
#endif

template <typename... Args>
void decode(std::byte*& buffer,
            QUILL_MAYBE_UNUSED fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store) noexcept
{
  (Decoder<Args>::decode(buffer, args_store), ...);
}

/**
 * Decoder functions
 */
using FormatArgsDecoder = void (*)(std::byte*& data,
                                   fmtquill::dynamic_format_arg_store<fmtquill::format_context>& args_store);

template <typename... Args>
void decode_and_populate_format_args(std::byte*& buffer,
                                     fmtquill::dynamic_format_arg_store<fmtquill::format_context>& args_store)
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
void decode_and_assign_members(std::byte*& buffer,
                               fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store,
                               T& arg, TMembers&... members)
{
  ((members = Decoder<remove_cvref_t<TMembers>>::decode(buffer, nullptr)), ...);

  if (args_store)
  {
    args_store->push_back(arg);
  }
}
} // namespace quill::detail
