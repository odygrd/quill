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

template <typename Arg>
constexpr size_t array_size_v = std::extent_v<remove_cvref_t<Arg>>;

template <class>
inline constexpr bool always_false_v = false;

constexpr auto strnlen =
#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
  ::strnlen_s
#else
  ::strnlen
#endif
  ;

/***/
template <typename Arg>
QUILL_NODISCARD constexpr bool is_char_array_type()
{
  using ArgType = remove_cvref_t<Arg>;
  return std::conjunction_v<std::is_array<ArgType>, std::is_same<remove_cvref_t<std::remove_extent_t<ArgType>>, char>>;
}

/***/
template <typename Arg>
QUILL_NODISCARD constexpr bool is_c_style_string_type()
{
  using ArgType = std::decay_t<Arg>;
  return !is_char_array_type<Arg>() &&
    std::disjunction_v<std::is_same<ArgType, char const*>, std::is_same<ArgType, char*>>;
}

/***/
template <typename Arg>
QUILL_NODISCARD constexpr bool is_std_string_type()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, std::string>, std::is_same<ArgType, std::string_view>>;
}

#if defined(_WIN32)
/**
 * Return the size required to encode a wide string
 * @param s wide string to be encoded
 * @return required size for encoding
 */
size_t inline get_wide_string_encoding_size(std::wstring_view s)
{
  return static_cast<size_t>(WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                                                 nullptr, 0, nullptr, nullptr));
}

/**
 * Converts a wide string to a narrow string
 */
void inline wide_string_to_narrow(void* dest, size_t required_bytes, std::wstring_view s)
{
  WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), static_cast<char*>(dest),
                      static_cast<int>(required_bytes), nullptr, nullptr);
}

/***/
template <typename Arg>
QUILL_NODISCARD constexpr bool is_c_style_wide_string_type()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, wchar_t const*>, std::is_same<ArgType, wchar_t*>>;
}

/***/
template <typename Arg>
QUILL_NODISCARD constexpr bool is_std_wstring_type()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, std::wstring>, std::is_same<ArgType, std::wstring_view>>;
}

/***/
template <typename... Args>
QUILL_NODISCARD constexpr uint32_t count_c_style_wide_strings() noexcept
{
  return (0u + ... + is_c_style_wide_string_type<Args>());
}

/***/
template <typename... Args>
QUILL_NODISCARD constexpr uint32_t count_std_wstring_type() noexcept
{
  return (0u + ... + is_std_wstring_type<Args>());
}
#endif

/***/
template <typename... Args>
QUILL_NODISCARD constexpr uint32_t count_c_style_strings() noexcept
{
  return (0u + ... + (is_char_array_type<Args>() || is_c_style_string_type<Args>()));
}

/***/
template <typename Arg>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline size_t calculate_arg_size_and_string_length(
  size_t* c_style_string_lengths, uint32_t& c_style_string_lengths_index, Arg const& arg) noexcept
{
  if constexpr (is_char_array_type<Arg>())
  {
    c_style_string_lengths[c_style_string_lengths_index] =
      static_cast<size_t>(strnlen(arg, array_size_v<Arg>) + 1u);

    return c_style_string_lengths[c_style_string_lengths_index++];
  }
  else if constexpr (is_c_style_string_type<Arg>())
  {
    // include one extra for the zero termination
    c_style_string_lengths[c_style_string_lengths_index] = static_cast<size_t>(strlen(arg) + 1u);

    return c_style_string_lengths[c_style_string_lengths_index++];
  }
  else if constexpr (is_std_string_type<Arg>())
  {
    // for std::string we also need to store the size in order to correctly retrieve it
    // the reason for this is that if we create e.g:
    // std::string msg = fmtquill::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
    // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
    return static_cast<size_t>(sizeof(size_t) + arg.length());
  }
  else if constexpr (std::is_arithmetic_v<Arg> || std::is_enum_v<Arg>)
  {
    return static_cast<size_t>(sizeof(arg));
  }
#if defined(_WIN32)
  else if constexpr (is_c_style_wide_string_type<Arg>())
  {
    c_style_string_lengths[c_style_string_lengths_index] =
      get_wide_string_encoding_size(std::wstring_view{arg, wcslen(arg)});
    return static_cast<size_t>(sizeof(size_t) + c_style_string_lengths[c_style_string_lengths_index++]);
  }
  else if constexpr (is_std_wstring_type<Arg>())
  {
    c_style_string_lengths[c_style_string_lengths_index] = get_wide_string_encoding_size(arg);
    return static_cast<size_t>(sizeof(size_t) + c_style_string_lengths[c_style_string_lengths_index++]);
  }
#endif
  else
  {
    static_assert(always_false_v<Arg>, "unsupported type");
  }
}

/**
 * @brief Calculates the total size required to encode the provided arguments and populates the c_style_string_lengths array.

 * @param c_style_string_lengths Array to store the c_style_string_lengths of C-style strings and char arrays.
 * @param args The arguments to be encoded.
 * @return The total size required to encode the arguments.
 */
template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline size_t calculate_args_size_and_populate_string_lengths(
  QUILL_MAYBE_UNUSED size_t* c_style_string_lengths, Args const&... args) noexcept
{
  QUILL_MAYBE_UNUSED uint32_t c_style_string_lengths_index{0};
  return (0u + ... + calculate_arg_size_and_string_length(c_style_string_lengths, c_style_string_lengths_index, args));
}

/***/
template <typename Arg>
QUILL_ATTRIBUTE_HOT inline void encode_arg(std::byte*& buffer, size_t const* c_style_string_lengths,
                                           uint32_t& c_style_string_lengths_index, Arg const& arg) noexcept
{
  if constexpr (is_char_array_type<Arg>())
  {
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
    constexpr auto array_size = array_size_v<Arg>;

    if (QUILL_UNLIKELY(len > array_size))
    {
      // no '\0' in c array
      assert(len == array_size + 1);
      std::memcpy(buffer, arg, array_size);
      buffer[len - 1] = std::byte{'\0'};
    }
    else
    {
      std::memcpy(buffer, arg, len);
    }

    buffer += len;
  }
  else if constexpr (is_c_style_string_type<Arg>())
  {
    // null terminator is included in the len for c style strings
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
    std::memcpy(buffer, arg, len);
    buffer += len;
  }
  else if constexpr (is_std_string_type<Arg>())
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
  else if constexpr (std::is_arithmetic_v<Arg> || std::is_enum_v<Arg>)
  {
    std::memcpy(buffer, &arg, sizeof(arg));
    buffer += sizeof(arg);
  }
#if defined(_WIN32)
  else if constexpr (is_c_style_wide_string_type<Arg>())
  {
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
    std::memcpy(buffer, &len, sizeof(len));

    if (len != 0)
    {
      wide_string_to_narrow(buffer + sizeof(size_t), len, std::wstring_view{arg, wcslen(arg)});
    }

    buffer += sizeof(size_t) + len;
  }
  else if constexpr (is_std_wstring_type<Arg>())
  {
    // for std::wstring we store the size first, in order to correctly retrieve it
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
    std::memcpy(buffer, &len, sizeof(len));

    if (len != 0)
    {
      wide_string_to_narrow(buffer + sizeof(size_t), len, arg);
    }

    buffer += sizeof(size_t) + len;
  }
#endif
  else
  {
    static_assert(always_false_v<Arg>, "unsupported type");
  }
}

/**
 * @brief Encodes multiple arguments into a buffer.
 * @param buffer Pointer to the buffer for encoding.
 * @param c_style_string_lengths Array storing the c_style_string_lengths of C-style strings and char arrays.
 * @param args The arguments to be encoded.
 */
template <typename... Args>
QUILL_ATTRIBUTE_HOT inline void encode(std::byte*& buffer, QUILL_MAYBE_UNUSED size_t const* c_style_string_lengths,
                                       Args const&... args) noexcept
{
  QUILL_MAYBE_UNUSED uint32_t c_style_string_lengths_index{0};
  (encode_arg(buffer, c_style_string_lengths, c_style_string_lengths_index, args), ...);
}

/***/
template <typename TFormatContext, typename Arg>
QUILL_ATTRIBUTE_HOT inline void decode_arg(std::byte*& buffer,
                                           std::vector<fmtquill::basic_format_arg<TFormatContext>>& args_store) noexcept
{
  if constexpr (is_c_style_string_type<Arg>() || is_char_array_type<Arg>())
  {
    char const* str = reinterpret_cast<char const*>(buffer);
    std::string_view const v{str, strlen(str)};
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));

    // for c_strings we add +1 to the length as we also want to copy the null terminated char
    buffer += v.length() + 1;
  }
  else if constexpr (is_std_string_type<Arg>())
  {
    // for std::string we first need to retrieve the length
    size_t len;
    std::memcpy(&len, buffer, sizeof(len));

    // retrieve the rest of the string
    char const* str = reinterpret_cast<char const*>(buffer + sizeof(size_t));
    std::string_view const v{str, len};
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));

    buffer += sizeof(len) + v.length();
  }
  else if constexpr (std::is_arithmetic_v<Arg> || std::is_enum_v<Arg>)
  {
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(*reinterpret_cast<Arg*>(buffer)));
    buffer += sizeof(Arg);
  }
#if defined(_WIN32)
  else if constexpr (is_c_style_wide_string_type<Arg>() || is_std_wstring_type<Arg>())
  {
    // for std::wstring we first need to retrieve the length
    size_t len;
    std::memcpy(&len, buffer, sizeof(len));

    char const* str = reinterpret_cast<char const*>(buffer + sizeof(size_t));
    std::string_view const v{str, len};
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));

    buffer += sizeof(len) + v.length();
  }
#endif
  else
  {
    static_assert(always_false_v<Arg>, "unsupported type");
  }
}

template <typename TFormatContext, typename... Args>
QUILL_ATTRIBUTE_HOT inline void decode(std::byte*& buffer,
                                       std::vector<fmtquill::basic_format_arg<TFormatContext>>& args_store) noexcept
{
  (decode_arg<TFormatContext, Args>(buffer, args_store), ...);
}

/**
 * Decode functions
 */
using FormatArgsDecoder = void (*)(std::byte*& data,
                                   std::vector<fmtquill::basic_format_arg<fmtquill::format_context>>& args_store);

template <typename... Args>
QUILL_ATTRIBUTE_HOT inline void decode_and_populate_format_args(
  std::byte*& buffer, std::vector<fmtquill::basic_format_arg<fmtquill::format_context>>& args_store)
{
  args_store.clear();
  decode<fmtquill::format_context, Args...>(buffer, args_store);
}
} // namespace quill::detail
