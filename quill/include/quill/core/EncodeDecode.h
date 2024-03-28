/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#ifndef __STDC_WANT_LIB_EXT1__
  #define __STDC_WANT_LIB_EXT1__ 1
  #include <cstring>
#endif

#include "quill/bundled/fmt/core.h"

#include "quill/core/Attributes.h"

#include <cstdint>
#include <cstring>
#include <cassert>
#include <string>
#include <string_view>
#include <type_traits>

namespace quill
{
namespace detail
{

/**
 * C++14 implementation of C++20's remove_cvref
 */
    template<class T>
    struct remove_cvref {
        typedef std::remove_cv_t<std::remove_reference_t<T>> type;
    };

    template<class T>
    using remove_cvref_t = typename remove_cvref<T>::type;

    template<typename Arg>
    constexpr size_t array_size_v = std::extent_v<remove_cvref_t<Arg>>;

// helper constant for the visitor #3
    template<class>
    inline constexpr bool always_false_v = false;

constexpr auto strnlen =
#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
  ::strnlen_s
#else
  ::strnlen
#endif
  ;

template <typename Arg>
QUILL_NODISCARD constexpr bool is_char_array_type()
{
  using ArgType = remove_cvref_t<Arg>;
  return std::conjunction_v<std::is_array<ArgType>, std::is_same<remove_cvref_t<std::remove_extent_t<ArgType>>, char>>;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_c_style_string_type()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, char const*>, std::is_same<ArgType, char*>>;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_std_string_type()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, std::string>, std::is_same<ArgType, std::string_view>>;
}

#if defined(_WIN32)
template <typename Arg>
QUILL_NODISCARD constexpr bool is_c_style_wide_string_type()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, wchar_t const*>, std::is_same<ArgType, wchar_t*>>;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_std_wstring_type()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, std::wstring>, std::is_same<ArgType, std::wstring_view>>;
}
#endif

template <typename TFormatContext, typename Arg>
QUILL_ATTRIBUTE_HOT inline void decode_arg(std::byte*& buffer,
                                           std::vector<fmtquill::basic_format_arg < TFormatContext>>

        & args_store) noexcept
{
  using ArgType = remove_cvref_t<Arg>;

        if constexpr (

        is_c_style_string_type<ArgType>()

        )
  {
    char const* str = reinterpret_cast<char const*>(buffer);
    std::string_view const v{str, strlen(str)};
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));

    // for c_strings we add +1 to the length as we also want to copy the null terminated char
    buffer += v.length() + 1;
  }
    else if constexpr (

    is_std_string_type<ArgType>()

    )
  {
    // for std::string we first need to retrieve the length
    size_t len;
    std::memcpy(&len, buffer, sizeof(size_t));

    // retrieve the rest of the string
    char const* str = reinterpret_cast<char const*>(buffer + sizeof(size_t));
    std::string_view const v{str, len};
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));

    buffer += sizeof(size_t) + v.length();
  }
#if defined(_WIN32)
else if constexpr (is_c_style_wide_string_type<ArgType>() || is_std_wstring_type<ArgType>())
{
  // for std::wstring we first need to retrieve the length
  size_t len;
  std::memcpy(&len, buffer, sizeof(size_t));

  char const* str = reinterpret_cast<char const*>(buffer + sizeof(size_t));
  std::string_view const v{str, len};
  args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));

  buffer += sizeof(size_t) + v.length();
}
#endif
  else
  {
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(*reinterpret_cast<ArgType*>(buffer)));
    buffer += sizeof(ArgType);
  }
}

template <typename TFormatContext, typename... Args>
QUILL_ATTRIBUTE_HOT inline void decode(std::byte*& buffer,
                                       std::vector<fmtquill::basic_format_arg < TFormatContext>>

& args_store) noexcept
{
(
decode_arg<TFormatContext, Args>(buffer, args_store
), ...);
}

template <typename... Args>
QUILL_NODISCARD constexpr uint32_t count_c_style_strings() noexcept
{
  return (0u + ... + is_c_style_string_type<Args>());
}

#if defined(_WIN32)
template <typename... Args>
QUILL_NODISCARD constexpr uint32_t count_c_style_wide_strings() noexcept
{
  return (0u + ... + is_c_style_wide_string_type<Args>());
}

template <typename... Args>
QUILL_NODISCARD constexpr uint32_t count_std_wstring_type() noexcept
{
  return (0u + ... + is_std_wstring_type<Args>());
}
#endif

/**
 * @brief Calculates the size of the argument and the length of the string representation if the argument is a string type.
 *
 * This function determines the size of the argument `arg` and the length of its string representation
 * if the argument is a string type, updating the `c_style_string_lengths` array accordingly.
 *
 * @param c_style_string_lengths An array to store the lengths of C-style strings and char arrays.
 * @param c_style_string_lengths_index The index used to update the `c_style_string_lengths` array.
 * @param arg The argument to calculate the size and length for.
 * @return The size of the argument and the length of its string representation if the argument is a string type.
 */
template<typename Arg>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline size_t calculate_arg_size_and_string_length(
        QUILL_MAYBE_UNUSED size_t

* c_style_string_lengths,
uint32_t &c_style_string_lengths_index, Arg
const& arg) noexcept
{
using ArgType = remove_cvref_t<Arg>;

if constexpr (

is_char_array_type<ArgType>()

)
  {
    c_style_string_lengths[c_style_string_lengths_index] =
static_cast
<size_t>(strnlen(arg, std::extent_v<ArgType>)
+ 1u);

    return c_style_string_lengths[c_style_string_lengths_index++];
  }
else if constexpr (

is_c_style_string_type<ArgType>()

)
  {
    // include one extra for the zero termination
    c_style_string_lengths[c_style_string_lengths_index] = static_cast<size_t>(strlen(arg) + 1u);

    return c_style_string_lengths[c_style_string_lengths_index++];
  }
else if constexpr (

is_std_string_type<ArgType>()

)
  {
    // for std::string we also need to store the size in order to correctly retrieve it
    // the reason for this is that if we create e.g:
    // std::string msg = fmtquill::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
    // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
return static_cast<size_t>(sizeof(size_t) + arg.

length()

);
  }
#if defined(_WIN32)
else if constexpr (is_c_style_wide_string_type<ArgType>())
{
  c_style_string_lengths[c_style_string_lengths_index] =
    get_wide_string_encoding_size(std::wstring_view{arg, wcslen(arg)});
  return static_cast<size_t>(sizeof(size_t) + c_style_string_lengths[c_style_string_lengths_index++]);
}
else if constexpr (is_std_wstring_type<ArgType>())
{
  c_style_string_lengths[c_style_string_lengths_index] = get_wide_string_encoding_size(arg);
  return static_cast<size_t>(sizeof(size_t) + c_style_string_lengths[c_style_string_lengths_index++]);
}
#endif
else if constexpr (std::is_arithmetic_v<ArgType>)
{
return static_cast<size_t>(sizeof(ArgType));
}
  else
  {
static_assert(
        always_false_v < ArgType > ,
        "Unsupported type detected. Only supports string and arithmetic types. "
        "Please convert the type to one of these supported types before passing it to the logger.");
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

/**
 * @brief Encodes an argument into a buffer.
 * @param buffer Pointer to the buffer for encoding.
 * @param c_style_string_lengths Array storing the c_style_string_lengths of C-style strings and char arrays.
 * @param c_style_string_lengths_index Index of the current string/array length in c_style_string_lengths.
 * @param arg The argument to be encoded.
 */
template<typename Arg>
QUILL_ATTRIBUTE_HOT inline void encode_arg(std::byte*& buffer, size_t const* c_style_string_lengths,
                                           uint32_t &c_style_string_lengths_index, Arg const &arg) noexcept
{
    using ArgType = remove_cvref_t<Arg>;

    if constexpr (is_char_array_type < ArgType > ())
  {
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
      constexpr auto array_size = array_size_v<ArgType>;
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
  } else if constexpr (is_c_style_string_type < ArgType > ())
  {
    // null terminator is included in the len for c style strings
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
    std::memcpy(buffer, arg, len);
    buffer += len;
  } else if constexpr (is_std_string_type < ArgType > ())
  {
    // for std::string we store the size first, in order to correctly retrieve it
    // Copy the length first and then the actual string
    size_t const len = arg.length();
    std::memcpy(buffer, &len, sizeof(size_t));

    if (len != 0)
    {
      // copy the string, no need to zero terminate it as we got the length
      std::memcpy(buffer + sizeof(size_t), arg.data(), arg.length());
    }

    buffer += sizeof(size_t) + len;
  }
#if defined(_WIN32)
        else if constexpr (is_c_style_wide_string_type<ArgType>())
        {
          size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
          std::memcpy(buffer, &len, sizeof(size_t));

          if (len != 0)
          {
            wide_string_to_narrow(buffer + sizeof(size_t), len, std::wstring_view{arg, wcslen(arg)});
          }

          buffer += sizeof(size_t) + len;
        }
        else if constexpr (is_std_wstring_type<ArgType>())
        {
          // for std::wstring we store the size first, in order to correctly retrieve it
          size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
          std::memcpy(buffer, &len, sizeof(size_t));

          if (len != 0)
          {
            wide_string_to_narrow(buffer + sizeof(size_t), len, arg);
          }

          buffer += sizeof(size_t) + len;
        }
#endif
  else
  {
      std::memcpy(buffer, &arg, sizeof(arg));
      buffer += sizeof(ArgType);
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

/**
 * Decode functions
 */
using FormatArgsDecoder = void (*)(std::byte *&data,
                                   std::vector<fmtquill::basic_format_arg < fmtquill::format_context>>& args_store);

template <typename... Args>
QUILL_ATTRIBUTE_HOT inline void decode_and_populate_format_args(
        std::byte *&buffer, std::vector<fmtquill::basic_format_arg < fmtquill::format_context>>

& args_store)
{
  args_store.clear();

decode<fmtquill::format_context, Args...>(buffer, args_store
);
}

} // namespace detail
} // namespace quill
