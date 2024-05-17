/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#ifndef __STDC_WANT_LIB_EXT1__
  #define __STDC_WANT_LIB_EXT1__ 1
  #include <cstring>
#endif

#include "misc/Utilities.h"
#include "quill/LogLevel.h"
#include "quill/MacroMetadata.h"
#include "quill/QuillError.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/TypeTraitsCopyable.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

namespace quill
{
namespace detail
{

constexpr auto strnlen =
#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
  ::strnlen_s
#else
  ::strnlen
#endif
  ;

/** Forward declaration **/
class LoggerDetails;

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

template <typename Arg>
QUILL_NODISCARD constexpr bool is_not_trivially_destructible()
{
  using ArgType = remove_cvref_t<Arg>;

#if defined(_WIN32)
  if constexpr (is_std_wstring_type<Arg>())
  {
    return false;
  }
#endif

  if constexpr (is_std_string_type<Arg>())
  {
    return false;
  }

  return !std::is_trivially_destructible_v<ArgType>;
}

template <typename TFormatContext, typename Arg>
QUILL_ATTRIBUTE_HOT inline void decode_arg(std::byte*& buffer,
                                           std::vector<fmtquill::basic_format_arg<TFormatContext>>& args_store,
                                           std::byte** destruct_args, uint32_t& destruct_arg_index) noexcept
{
  using ArgType = remove_cvref_t<Arg>;

  if constexpr (is_c_style_string_type<Arg>())
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
    buffer = align_pointer<alignof(size_t), std::byte>(buffer);
    size_t len;
    std::memcpy(&len, buffer, sizeof(size_t));

    // retrieve the rest of the string
    char const* str = reinterpret_cast<char const*>(buffer + sizeof(size_t));
    std::string_view const v{str, len};
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));

    buffer += sizeof(size_t) + v.length();
  }
#if defined(_WIN32)
  else if constexpr (is_c_style_wide_string_type<Arg>() || is_std_wstring_type<Arg>())
  {
    // for std::wstring we first need to retrieve the length
    buffer = align_pointer<alignof(size_t), std::byte>(buffer);
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
    // no need to align for chars, but align for any other type
    buffer = align_pointer<alignof(Arg), std::byte>(buffer);
    args_store.emplace_back(fmtquill::detail::make_arg<TFormatContext>(*reinterpret_cast<ArgType*>(buffer)));

    if constexpr (is_not_trivially_destructible<Arg>())
    {
      destruct_args[destruct_arg_index++] = buffer;
    }

    buffer += sizeof(ArgType);
  }
}

template <typename TFormatContext, typename... Args>
QUILL_ATTRIBUTE_HOT inline void decode(std::byte*& buffer,
                                       std::vector<fmtquill::basic_format_arg<TFormatContext>>& args_store,
                                       QUILL_MAYBE_UNUSED std::byte** destruct_args) noexcept
{
  QUILL_MAYBE_UNUSED uint32_t destruct_arg_index{0};
  (decode_arg<TFormatContext, Args>(buffer, args_store, destruct_args, destruct_arg_index), ...);
}

template <typename Arg>
QUILL_ATTRIBUTE_HOT inline void destruct_arg(std::byte** destruct_args, uint32_t& destruct_arg_index) noexcept
{
  using ArgType = remove_cvref_t<Arg>;
  if constexpr (is_not_trivially_destructible<Arg>())
  {
    reinterpret_cast<ArgType*>(destruct_args[destruct_arg_index++])->~ArgType();
  }
}

template <typename... Args>
QUILL_ATTRIBUTE_HOT inline void destruct(QUILL_MAYBE_UNUSED std::byte** destruct_args)
{
  QUILL_MAYBE_UNUSED uint32_t destruct_arg_index{0};
  (destruct_arg<Args>(destruct_args, destruct_arg_index), ...);
}

template <typename... Args>
QUILL_NODISCARD constexpr uint32_t count_c_style_strings() noexcept
{
  return (0u + ... + is_c_style_string_type<Args>());
}

template <typename... Args>
QUILL_NODISCARD constexpr uint32_t count_not_trivially_destructible_args() noexcept
{
  return (0u + ... + is_not_trivially_destructible<Args>());
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
template <typename T>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline size_t calculate_arg_size_and_string_length(
  QUILL_MAYBE_UNUSED size_t* c_style_string_lengths, uint32_t& c_style_string_lengths_index, T const& arg) noexcept
{
  if constexpr (is_char_array_type<T>())
  {
    c_style_string_lengths[c_style_string_lengths_index] =
      static_cast<size_t>(strnlen(arg, std::extent_v<T>) + 1u);

    return c_style_string_lengths[c_style_string_lengths_index++];
  }
  else if constexpr (is_c_style_string_type<T>())
  {
    // include one extra for the zero termination
    c_style_string_lengths[c_style_string_lengths_index] = static_cast<size_t>(strlen(arg) + 1u);

    return c_style_string_lengths[c_style_string_lengths_index++];
  }
  else if constexpr (is_std_string_type<T>())
  {
    // for std::string we also need to store the size in order to correctly retrieve it
    // the reason for this is that if we create e.g:
    // std::string msg = fmtquill::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
    // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
    return static_cast<size_t>(sizeof(size_t) + alignof(size_t) + arg.length());
  }
#if defined(_WIN32)
  else if constexpr (is_c_style_wide_string_type<T>())
  {
    c_style_string_lengths[c_style_string_lengths_index] =
      get_wide_string_encoding_size(std::wstring_view{arg, wcslen(arg)});
    return static_cast<size_t>(sizeof(size_t) + alignof(size_t) +
                               c_style_string_lengths[c_style_string_lengths_index++]);
  }
  else if constexpr (is_std_wstring_type<T>())
  {
    c_style_string_lengths[c_style_string_lengths_index] = get_wide_string_encoding_size(arg);
    return static_cast<size_t>(sizeof(size_t) + alignof(size_t) +
                               c_style_string_lengths[c_style_string_lengths_index++]);
  }
#endif
  else
  {
    return static_cast<size_t>(alignof(T) + sizeof(T));
  }
};

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
  // Do not use a fold expression for sequential evaluation
  QUILL_MAYBE_UNUSED uint32_t c_style_string_lengths_index{0};
  size_t total_size{0};
  ((total_size += calculate_arg_size_and_string_length(c_style_string_lengths, c_style_string_lengths_index, args)),
   ...);
  return total_size;
}

/**
 * @brief Encodes an argument into a buffer.
 * @param buffer Pointer to the buffer for encoding.
 * @param c_style_string_lengths Array storing the c_style_string_lengths of C-style strings and char arrays.
 * @param c_style_string_lengths_index Index of the current string/array length in c_style_string_lengths.
 * @param arg The argument to be encoded.
 */
template <typename T>
QUILL_ATTRIBUTE_HOT inline void encode_arg(std::byte*& buffer, size_t const* c_style_string_lengths,
                                           uint32_t& c_style_string_lengths_index, T const& arg) noexcept
{
  if constexpr (is_char_array_type<T>())
  {
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
    constexpr auto array_size = array_size_v<T>;
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
  else if constexpr (is_c_style_string_type<T>())
  {
    // null terminator is included in the len for c style strings
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
    std::memcpy(buffer, arg, len);
    buffer += len;
  }
  else if constexpr (is_std_string_type<T>())
  {
    // for std::string we store the size first, in order to correctly retrieve it
    // Copy the length first and then the actual string
    buffer = align_pointer<alignof(size_t), std::byte>(buffer);
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
  else if constexpr (is_c_style_wide_string_type<T>())
  {
    buffer = align_pointer<alignof(size_t), std::byte>(buffer);
    size_t const len = c_style_string_lengths[c_style_string_lengths_index++];
    std::memcpy(buffer, &len, sizeof(size_t));

    if (len != 0)
    {
      wide_string_to_narrow(buffer + sizeof(size_t), len, std::wstring_view{arg, wcslen(arg)});
    }

    buffer += sizeof(size_t) + len;
  }
  else if constexpr (is_std_wstring_type<T>())
  {
    // for std::wstring we store the size first, in order to correctly retrieve it
    buffer = align_pointer<alignof(size_t), std::byte>(buffer);
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
    // no need to align for chars, but align for any other type
    buffer = align_pointer<alignof(T), std::byte>(buffer);

    // use memcpy when possible
    if constexpr (std::is_trivially_copyable_v<remove_cvref_t<T>>)
    {
      std::memcpy(buffer, &arg, sizeof(arg));
    }
    else
    {
      new (buffer) remove_cvref_t<T>(arg);
    }

    buffer += sizeof(T);
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
 * Format function
 */
using FormatToFn = bool (*)(std::string_view format, std::byte*& data, transit_event_fmt_buffer_t& out,
                            std::vector<fmtquill::basic_format_arg<fmtquill::format_context>>& args_store,
                            std::vector<std::pair<std::string, transit_event_fmt_buffer_t>>* structured_kvs);
using PrintfFormatToFn = bool (*)(std::string_view format, std::byte*& data, transit_event_fmt_buffer_t& out,
                                  std::vector<fmtquill::basic_format_arg<fmtquill::printf_context>>& args_store);

template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool format_to(
  std::string_view format, std::byte*& data, transit_event_fmt_buffer_t& out,
  std::vector<fmtquill::basic_format_arg<fmtquill::format_context>>& args_store,
  std::vector<std::pair<std::string, transit_event_fmt_buffer_t>>* structured_kvs)
{
  bool success{true};

  constexpr size_t num_dtors = count_not_trivially_destructible_args<Args...>();
  std::byte* dtor_args[(std::max)(num_dtors, static_cast<size_t>(1))];

  decode<fmtquill::format_context, Args...>(data, args_store, dtor_args);

  out.clear();

  QUILL_TRY
  {
    fmtquill::vformat_to(std::back_inserter(out), format,
                         fmtquill::basic_format_args(args_store.data(), sizeof...(Args)));

    if (structured_kvs)
    {
      // if we are processing a structured log we need to pass the values of the args here
      // At the end of the function we will be destructing the args
      for (size_t i = 0; i < args_store.size(); ++i)
      {
        fmtquill::vformat_to(std::back_inserter((*structured_kvs)[i].second), "{}",
                             fmtquill::basic_format_args(&args_store[i], 1));
      }
    }
  }
#if !defined(QUILL_NO_EXCEPTIONS)
  QUILL_CATCH(std::exception const& e)
  {
    out.clear();
    std::string const error = fmtquill::format(
      "[Could not format log statement. message: \"{}\", error: \"{}\"]", format, e.what());
    out.append(error.data(), error.data() + error.length());
    success = false;
  }
#endif

  destruct<Args...>(dtor_args);
  args_store.clear();

  return success;
}

template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT bool printf_format_to(
  std::string_view format, std::byte*& data, transit_event_fmt_buffer_t& out,
  std::vector<fmtquill::basic_format_arg<fmtquill::printf_context>>& args_store)
{
  bool success{true};

  constexpr size_t num_dtors = count_not_trivially_destructible_args<Args...>();
  std::byte* dtor_args[(std::max)(num_dtors, static_cast<size_t>(1))];

  decode<fmtquill::printf_context, Args...>(data, args_store, dtor_args);

  out.clear();

  QUILL_TRY
  {
    fmtquill::detail::vprintf(out, fmtquill::basic_string_view<char>{format.data(), format.length()},
                              fmtquill::printf_args(args_store.data(), sizeof...(Args)));
  }
#if !defined(QUILL_NO_EXCEPTIONS)
  QUILL_CATCH(std::exception const& e)
  {
    out.clear();
    std::string const error = fmtquill::format(
      "[Could not format log statement. message: \"{}\", error: \"{}\"]", format, e.what());
    out.append(error.data(), error.data() + error.length());
    success = false;
  }
#endif

  destruct<Args...>(dtor_args);
  args_store.clear();

  return success;
}

} // namespace detail
} // namespace quill
