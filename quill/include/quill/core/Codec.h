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
inline constexpr bool always_false_v = false;

constexpr auto strnlen =
#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
  ::strnlen_s
#else
  ::strnlen
#endif
  ;

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
#endif

/** typename = void for specializations with enable_if **/
template <typename Arg, typename = void>
struct ArgSizeCalculator
{
  QUILL_ATTRIBUTE_HOT static inline size_t calculate(std::vector<size_t>&, Arg const&) noexcept
  {
    static_assert(always_false_v<Arg>, "Unsupported type");
    return 0;
  }
};

/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>>>
{
  QUILL_ATTRIBUTE_HOT static constexpr size_t calculate(std::vector<size_t>&, Arg) noexcept
  {
    return static_cast<size_t>(sizeof(Arg));
  }
};

/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>>>>
{
  QUILL_ATTRIBUTE_HOT static inline size_t calculate(std::vector<size_t>& c_style_string_length_cache,
                                                     char const* arg) noexcept
  {
    // include one extra for the zero termination
    c_style_string_length_cache.push_back(static_cast<size_t>(strlen(arg) + 1u));
    return c_style_string_length_cache.back();
  }
};

/***/
template <std::size_t N>
struct ArgSizeCalculator<char[N]>
{
  QUILL_ATTRIBUTE_HOT static inline size_t calculate(std::vector<size_t>& c_style_string_length_cache,
                                                     char const (&arg)[N]) noexcept
  {
    c_style_string_length_cache.push_back(static_cast<size_t>(strnlen(arg, N) + 1u));
    return c_style_string_length_cache.back();
  }
};

/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::string>, std::is_same<Arg, std::string_view>>>>
{
  QUILL_ATTRIBUTE_HOT static inline size_t calculate(std::vector<size_t>&, Arg const& arg) noexcept
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
  QUILL_ATTRIBUTE_HOT static inline size_t calculate(std::vector<size_t>& c_style_string_length_cache,
                                                     wchar_t const* arg) noexcept
  {
    // Those strings won't be zero terminated after encoding, we will zero terminate them ourselves
    // so we add + 1 to the size
    c_style_string_length_cache.push_back(
      get_wide_string_encoding_size(std::wstring_view{arg, wcslen(arg)}) + 1u);
    return static_cast<size_t>(c_style_string_length_cache.back());
  }
};

/***/
template <typename Arg>
struct ArgSizeCalculator<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>>>
{
  QUILL_ATTRIBUTE_HOT static inline size_t calculate(std::vector<size_t>& c_style_string_length_cache,
                                                     Arg const& arg) noexcept
  {
    c_style_string_length_cache.push_back(get_wide_string_encoding_size(arg));
    return static_cast<size_t>(sizeof(size_t) + c_style_string_length_cache.back());
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
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline size_t calculate_args_size_and_populate_string_lengths(
  QUILL_MAYBE_UNUSED std::vector<size_t>& c_style_string_length_cache, Args const&... args) noexcept
{
  // Do not use fold expression with '+ ...' as we need a guaranteed sequence for the args here
  size_t total_sum{0};
  ((total_sum += ArgSizeCalculator<remove_cvref_t<Args>>::calculate(c_style_string_length_cache, args)), ...);
  return total_sum;
}

/** typename = void for specializations with enable_if **/
template <typename Arg, typename = void>
struct Encoder
{
  QUILL_ATTRIBUTE_HOT static inline void encode(std::byte*&, std::vector<size_t> const&, uint32_t&, Arg const&) noexcept
  {
    static_assert(always_false_v<Arg>, "Unsupported type");
  }
};

/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>>>
{
  QUILL_ATTRIBUTE_HOT static inline void encode(std::byte*& buffer, std::vector<size_t> const&,
                                                uint32_t&, Arg arg) noexcept
  {
    std::memcpy(buffer, &arg, sizeof(arg));
    buffer += sizeof(arg);
  }
};

/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, char*>, std::is_same<Arg, char const*>>>>
{
  QUILL_ATTRIBUTE_HOT static inline void encode(std::byte*& buffer, std::vector<size_t> const& c_style_string_lengths_cache,
                                                uint32_t& c_style_string_lengths_cache_index,
                                                char const* arg) noexcept
  {
    // null terminator is included in the len for c style strings
    size_t const len = c_style_string_lengths_cache[c_style_string_lengths_cache_index++];
    std::memcpy(buffer, arg, len);
    buffer += len;
  }
};

/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::string>, std::is_same<Arg, std::string_view>>>>
{
  QUILL_ATTRIBUTE_HOT static inline void encode(std::byte*& buffer, std::vector<size_t> const&,
                                                uint32_t&, Arg const& arg) noexcept
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
template <std::size_t N>
struct Encoder<char[N]>
{
  QUILL_ATTRIBUTE_HOT static inline void encode(std::byte*& buffer, std::vector<size_t> const& c_style_string_lengths_cache,
                                                uint32_t& c_style_string_lengths_cache_index,
                                                char const (&arg)[N]) noexcept
  {
    size_t const len = c_style_string_lengths_cache[c_style_string_lengths_cache_index++];

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
  QUILL_ATTRIBUTE_HOT static inline void encode(std::byte*& buffer, std::vector<size_t> const& c_style_string_lengths_cache,
                                                uint32_t& c_style_string_lengths_cache_index,
                                                wchar_t const* arg) noexcept
  {
    size_t const total_len = c_style_string_lengths_cache[c_style_string_lengths_cache_index++];
    size_t const str_len = total_len - 1; // excluding the zero termination

    if (str_len != 0)
    {
      wide_string_to_narrow(buffer, str_len, std::wstring_view{arg, wcslen(arg)});
      buffer += str_len;
    }

    constexpr char zero_term = '\0';
    std::memcpy(buffer, &zero_term, sizeof(zero_term));
    buffer += sizeof(zero_term);
  }
};

/***/
template <typename Arg>
struct Encoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>>>
{
  QUILL_ATTRIBUTE_HOT static inline void encode(std::byte*& buffer, std::vector<size_t> const& c_style_string_lengths_cache,
                                                uint32_t& c_style_string_lengths_cache_index,
                                                Arg const& arg) noexcept
  {
    // for std::wstring we store the size first, in order to correctly retrieve it
    size_t const len = c_style_string_lengths_cache[c_style_string_lengths_cache_index++];
    std::memcpy(buffer, &len, sizeof(len));

    if (len != 0)
    {
      wide_string_to_narrow(buffer + sizeof(size_t), len, arg);
    }

    buffer += sizeof(size_t) + len;
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
QUILL_ATTRIBUTE_HOT inline void encode(std::byte*& buffer, std::vector<size_t> const& c_style_string_length_cache,
                                       Args const&... args) noexcept
{
  QUILL_MAYBE_UNUSED uint32_t c_style_string_lengths_cache_index{0};
  (Encoder<Args>::encode(buffer, c_style_string_length_cache, c_style_string_lengths_cache_index, args), ...);
}

/** typename = void for specializations with enable_if **/
template <typename Arg, typename = void>
struct Decoder
{
  QUILL_ATTRIBUTE_HOT static inline void decode(std::byte*&,
                                                fmtquill::dynamic_format_arg_store<fmtquill::format_context>*)
  {
    static_assert(always_false_v<Arg>, "Unsupported type");
  }
};

/***/
template <typename Arg>
struct Decoder<Arg, std::enable_if_t<std::disjunction_v<std::is_arithmetic<Arg>, std::is_enum<Arg>>>>
{
  QUILL_ATTRIBUTE_HOT static inline Arg decode(std::byte*& buffer,
                                               fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
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
  QUILL_ATTRIBUTE_HOT static inline char const* decode(
    std::byte*& buffer, fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
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
  QUILL_ATTRIBUTE_HOT static inline std::string_view decode(
    std::byte*& buffer, fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
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
template <std::size_t N>
struct Decoder<char[N]>
{
  QUILL_ATTRIBUTE_HOT static inline char const* decode(
    std::byte*& buffer, fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
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
  QUILL_ATTRIBUTE_HOT static inline char const* decode(
    std::byte*& buffer, fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
  {
    char const* str = reinterpret_cast<char const*>(buffer);
    buffer += strlen(str) + 1; // include the zero terminator

    if (args_store)
    {
      args_store->push_back(str);
    }

    return str;
  }
};

/***/
template <typename Arg>
struct Decoder<Arg, std::enable_if_t<std::disjunction_v<std::is_same<Arg, std::wstring>, std::is_same<Arg, std::wstring_view>>>>
{
  QUILL_ATTRIBUTE_HOT static inline std::string_view decode(
    std::byte*& buffer, fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store)
  {
    // for std::wstring we first need to retrieve the length
    size_t len;
    std::memcpy(&len, buffer, sizeof(len));

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
#endif

template <typename... Args>
QUILL_ATTRIBUTE_HOT inline void decode(
  std::byte*& buffer, QUILL_MAYBE_UNUSED fmtquill::dynamic_format_arg_store<fmtquill::format_context>* args_store) noexcept
{
  (Decoder<Args>::decode(buffer, args_store), ...);
}

/**
 * Decoder functions
 */
using FormatArgsDecoder = void (*)(std::byte*& data,
                                   fmtquill::dynamic_format_arg_store<fmtquill::format_context>& args_store);

template <typename... Args>
QUILL_ATTRIBUTE_HOT inline void decode_and_populate_format_args(
  std::byte*& buffer, fmtquill::dynamic_format_arg_store<fmtquill::format_context>& args_store)
{
  args_store.clear();
  decode<Args...>(buffer, &args_store);
}
} // namespace quill::detail
