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
#ifdef __STDC_LIB_EXT1__
  ::strnlen_s
#else
  ::strnlen
#endif
  ;

/** Forward declaration **/
class LoggerDetails;

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_c_array()
{
  using ArgType = remove_cvref_t<Arg>;
  return std::is_array_v<ArgType> && std::is_same_v<remove_cvref_t<std::remove_extent_t<ArgType>>, char>;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_c_string()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, char const*>, std::is_same<ArgType, char*>>;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_string()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, std::string>, std::is_same<ArgType, std::string_view>>;
}

#if defined(_WIN32)
template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_wide_c_string()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, wchar_t const*>, std::is_same<ArgType, wchar_t*>>;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_wide_string()
{
  using ArgType = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<ArgType, std::wstring>, std::is_same<ArgType, std::wstring_view>>;
}
#endif

template <typename Arg>
QUILL_NODISCARD constexpr bool need_call_dtor_for()
{
  using ArgType = remove_cvref_t<Arg>;

#if defined(_WIN32)
  if constexpr (is_type_of_wide_string<Arg>())
  {
    return false;
  }
#endif

  if constexpr (is_type_of_string<Arg>())
  {
    return false;
  }

  return !std::is_trivially_destructible_v<ArgType>;
}

template <typename TFormatContext, size_t DestructIdx>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* decode_args(
  std::byte* in, std::vector<fmtquill::basic_format_arg<TFormatContext>>&, std::byte**)
{
  return in;
}

template <typename TFormatContext, size_t DestructIdx, typename Arg, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* decode_args(
  std::byte* in, std::vector<fmtquill::basic_format_arg<TFormatContext>>& args, std::byte** destruct_args)
{
  using ArgType = remove_cvref_t<Arg>;

  if constexpr (is_type_of_c_string<Arg>())
  {
    char const* str = reinterpret_cast<char const*>(in);
    std::string_view const v{str, strlen(str)};
    args.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));

    // for c_strings we add +1 to the length as we also want to copy the null terminated char
    return decode_args<TFormatContext, DestructIdx, Args...>(in + v.length() + 1, args, destruct_args);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    // for std::string we first need to retrieve the length
    in = align_pointer<alignof(size_t), std::byte>(in);
    size_t len{0};
    std::memcpy(&len, in, sizeof(size_t));
    in += sizeof(size_t);

    // retrieve the rest of the string
    char const* str = reinterpret_cast<char const*>(in);
    std::string_view const v{str, len};
    args.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));
    return decode_args<TFormatContext, DestructIdx, Args...>(in + v.length(), args, destruct_args);
  }
#if defined(_WIN32)
  else if constexpr (is_type_of_wide_c_string<Arg>() || is_type_of_wide_string<Arg>())
  {
    // for std::wstring we first need to retrieve the length
    in = align_pointer<alignof(size_t), std::byte>(in);
    size_t len{0};
    std::memcpy(&len, in, sizeof(size_t));
    in += sizeof(size_t);

    char const* str = reinterpret_cast<char const*>(in);
    std::string_view const v{str, len};
    args.emplace_back(fmtquill::detail::make_arg<TFormatContext>(v));
    return decode_args<TFormatContext, DestructIdx, Args...>(in + v.length(), args, destruct_args);
  }
#endif
  else
  {
    // no need to align for chars, but align for any other type
    in = align_pointer<alignof(Arg), std::byte>(in);

    args.emplace_back(fmtquill::detail::make_arg<TFormatContext>(*reinterpret_cast<ArgType*>(in)));

    if constexpr (need_call_dtor_for<Arg>())
    {
      destruct_args[DestructIdx] = in;
      return decode_args<TFormatContext, DestructIdx + 1, Args...>(in + sizeof(ArgType), args, destruct_args);
    }
    else
    {
      return decode_args<TFormatContext, DestructIdx, Args...>(in + sizeof(ArgType), args, destruct_args);
    }
  }
}

template <size_t DestructIdx>
QUILL_ATTRIBUTE_HOT void destruct_args(std::byte**)
{
}

template <size_t DestructIdx, typename Arg, typename... Args>
QUILL_ATTRIBUTE_HOT void destruct_args(std::byte** args)
{
  using ArgType = remove_cvref_t<Arg>;
  if constexpr (need_call_dtor_for<Arg>())
  {
    reinterpret_cast<ArgType*>(args[DestructIdx])->~ArgType();
    destruct_args<DestructIdx + 1, Args...>(args);
  }
  else
  {
    destruct_args<DestructIdx, Args...>(args);
  }
}

template <size_t CstringIdx>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT constexpr size_t get_args_sizes(size_t*)
{
  return 0;
}

/**
 * Get the size of all arguments
 */
template <size_t CstringIdx, typename Arg, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT constexpr size_t get_args_sizes(size_t* c_string_sizes,
                                                                    Arg const& arg, Args const&... args)
{
  if constexpr (is_type_of_c_array<Arg>())
  {
    size_t const len = strnlen(arg, array_size_v<Arg>) + 1;
    c_string_sizes[CstringIdx] = len;
    return len + get_args_sizes<CstringIdx + 1>(c_string_sizes, args...);
  }
  else if constexpr (is_type_of_c_string<Arg>())
  {
    size_t const len = strlen(arg) + 1;
    c_string_sizes[CstringIdx] = len;
    return len + get_args_sizes<CstringIdx + 1>(c_string_sizes, args...);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    // for std::string we also need to store the size in order to correctly retrieve it
    // the reason for this is that if we create e.g:
    // std::string msg = fmtquill::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
    // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
    return (arg.size() + sizeof(size_t)) + alignof(size_t) +
      get_args_sizes<CstringIdx>(c_string_sizes, args...);
  }
#if defined(_WIN32)
  else if constexpr (is_type_of_wide_c_string<Arg>())
  {
    size_t const len = get_wide_string_encoding_size(std::wstring_view{arg, wcslen(arg)});
    c_string_sizes[CstringIdx] = len;
    return len + sizeof(size_t) + alignof(size_t) + get_args_sizes<CstringIdx + 1>(c_string_sizes, args...);
  }
  else if constexpr (is_type_of_wide_string<Arg>())
  {
    size_t const len = get_wide_string_encoding_size(arg);
    c_string_sizes[CstringIdx] = len;
    return len + sizeof(size_t) + alignof(size_t) + get_args_sizes<CstringIdx + 1>(c_string_sizes, args...);
  }
#endif
  else
  {
    return alignof(Arg) + sizeof(Arg) + get_args_sizes<CstringIdx>(c_string_sizes, args...);
  }
}

/**
 * Encode args to the buffer
 */
template <size_t CstringIdx>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT constexpr std::byte* encode_args(size_t*, std::byte* out)
{
  return out;
}

template <size_t CstringIdx, typename Arg, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT constexpr std::byte* encode_args(size_t* c_string_sizes, std::byte* out,
                                                                     Arg&& arg, Args&&... args)
{
  if constexpr (is_type_of_c_array<Arg>())
  {
    const auto size = c_string_sizes[CstringIdx];
    constexpr auto array_size = array_size_v<Arg>;
    if (QUILL_UNLIKELY(size > array_size))
    {
      // no '\0' in c array
      assert(size == array_size + 1);
      std::memcpy(out, arg, array_size);
      out[size - 1] = std::byte{'\0'};
    }
    else
    {
      std::memcpy(out, arg, size);
    }
    return encode_args<CstringIdx + 1>(c_string_sizes, out + size, std::forward<Args>(args)...);
  }
  else if constexpr (is_type_of_c_string<Arg>())
  {
    assert((c_string_sizes[CstringIdx] > 0) &&
           "we include the null terminated char to the size of the string for C strings");
    std::memcpy(out, arg, c_string_sizes[CstringIdx]);
    return encode_args<CstringIdx + 1>(c_string_sizes, out + c_string_sizes[CstringIdx],
                                       std::forward<Args>(args)...);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    // for std::string we store the size first, in order to correctly retrieve it
    out = align_pointer<alignof(size_t), std::byte>(out);
    size_t const len = arg.length();
    std::memcpy(out, &len, sizeof(size_t));
    out += sizeof(size_t);

    if (len != 0)
    {
      // copy the string, no need to zero terminate it as we got the length
      std::memcpy(out, arg.data(), arg.length());
    }

    return encode_args<CstringIdx>(c_string_sizes, out + arg.length(), std::forward<Args>(args)...);
  }
#if defined(_WIN32)
  else if constexpr (is_type_of_wide_c_string<Arg>())
  {
    out = align_pointer<alignof(size_t), std::byte>(out);
    size_t const len = c_string_sizes[CstringIdx];
    std::memcpy(out, &len, sizeof(size_t));
    out += sizeof(size_t);

    if (c_string_sizes[CstringIdx] != 0)
    {
      wide_string_to_narrow(out, c_string_sizes[CstringIdx], std::wstring_view{arg, wcslen(arg)});
    }

    return encode_args<CstringIdx + 1>(c_string_sizes, out + c_string_sizes[CstringIdx],
                                       std::forward<Args>(args)...);
  }
  else if constexpr (is_type_of_wide_string<Arg>())
  {
    // for std::wstring we store the size first, in order to correctly retrieve it
    out = align_pointer<alignof(size_t), std::byte>(out);
    size_t const len = c_string_sizes[CstringIdx];
    std::memcpy(out, &len, sizeof(size_t));
    out += sizeof(size_t);

    if (c_string_sizes[CstringIdx] != 0)
    {
      wide_string_to_narrow(out, c_string_sizes[CstringIdx], arg);
    }

    return encode_args<CstringIdx + 1>(c_string_sizes, out + c_string_sizes[CstringIdx],
                                       std::forward<Args>(args)...);
  }
#endif
  else
  {
    // no need to align for chars, but align for any other type
    out = align_pointer<alignof(Arg), std::byte>(out);

    // use memcpy when possible
    if constexpr (std::is_trivially_copyable_v<remove_cvref_t<Arg>>)
    {
      std::memcpy(out, &arg, sizeof(Arg));
    }
    else
    {
      new (out) remove_cvref_t<Arg>(std::forward<Arg>(arg));
    }

    return encode_args<CstringIdx>(c_string_sizes, out + sizeof(Arg), std::forward<Args>(args)...);
  }
}

/**
 * Format function
 */
using FormatToFn = std::pair<std::byte*, std::string> (*)(
  std::string_view format, std::byte* data, transit_event_fmt_buffer_t& out,
  std::vector<fmtquill::basic_format_arg<fmtquill::format_context>>& args,
  std::vector<std::pair<std::string, transit_event_fmt_buffer_t>>* structured_kvs);
using PrintfFormatToFn = std::pair<std::byte*, std::string> (*)(
  std::string_view format, std::byte* data, transit_event_fmt_buffer_t& out,
  std::vector<fmtquill::basic_format_arg<fmtquill::printf_context>>& args);

template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::pair<std::byte*, std::string> format_to(
  std::string_view format, std::byte* data, transit_event_fmt_buffer_t& out,
  std::vector<fmtquill::basic_format_arg<fmtquill::format_context>>& args,
  std::vector<std::pair<std::string, transit_event_fmt_buffer_t>>* structured_kvs)
{
  std::string error;
  constexpr size_t num_dtors = fmtquill::detail::count<need_call_dtor_for<Args>()...>();
  std::byte* dtor_args[(std::max)(num_dtors, static_cast<size_t>(1))];

  std::byte* ret = decode_args<fmtquill::format_context, 0, Args...>(data, args, dtor_args);

  out.clear();

  QUILL_TRY
  {
    fmtquill::vformat_to(std::back_inserter(out), format,
                         fmtquill::basic_format_args(args.data(), sizeof...(Args)));

    if (structured_kvs)
    {
      // if we are processing a structured log we need to pass the values of the args here
      // At the end of the function we will be destructing the args
      for (size_t i = 0; i < args.size(); ++i)
      {
        fmtquill::vformat_to(std::back_inserter((*structured_kvs)[i].second), "{}",
                             fmtquill::basic_format_args(&args[i], 1));
      }
    }
  }
#if !defined(QUILL_NO_EXCEPTIONS)
  QUILL_CATCH(std::exception const& e)
  {
    out.clear();
    error = fmtquill::format("[format: \"{}\", error: \"{}\"]", format, e.what());
    out.append(error.data(), error.data() + error.length());
  }
#endif

  destruct_args<0, Args...>(dtor_args);
  args.clear();

  return std::make_pair(ret, std::move(error));
}

template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::pair<std::byte*, std::string> printf_format_to(
  std::string_view format, std::byte* data, transit_event_fmt_buffer_t& out,
  std::vector<fmtquill::basic_format_arg<fmtquill::printf_context>>& args)
{
  std::string error;
  constexpr size_t num_dtors = fmtquill::detail::count<need_call_dtor_for<Args>()...>();
  std::byte* dtor_args[(std::max)(num_dtors, static_cast<size_t>(1))];

  std::byte* ret = decode_args<fmtquill::printf_context, 0, Args...>(data, args, dtor_args);

  out.clear();

  QUILL_TRY
  {
    fmtquill::detail::vprintf(out, fmtquill::basic_string_view<char>{format.data(), format.length()},
                              fmtquill::printf_args(args.data(), sizeof...(Args)));
  }
#if !defined(QUILL_NO_EXCEPTIONS)
  QUILL_CATCH(std::exception const& e)
  {
    out.clear();
    error = fmtquill::format("[format: \"{}\", error: \"{}\"]", format, e.what());
    out.append(error.data(), error.data() + error.length());
  }
#endif

  destruct_args<0, Args...>(dtor_args);
  args.clear();

  return std::make_pair(ret, std::move(error));
}

/**
 * This function pointer is used to store and pass the template parameters to the backend worker
 * thread
 */
using MetadataFormatFn = std::pair<MacroMetadata, std::pair<FormatToFn, PrintfFormatToFn>> (*)();

template <bool IsPrintfFormat, typename TAnonymousStruct, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT constexpr std::pair<MacroMetadata, std::pair<FormatToFn, PrintfFormatToFn>> get_metadata_and_format_fn()
{
  if constexpr (!IsPrintfFormat)
  {
    constexpr auto ret = std::make_pair(TAnonymousStruct{}(), std::make_pair(format_to<Args...>, nullptr));
    return ret;
  }
  else
  {
    constexpr auto ret =
      std::make_pair(TAnonymousStruct{}(), std::make_pair(nullptr, printf_format_to<Args...>));
    return ret;
  }
}

} // namespace detail

namespace detail
{
struct Header
{
public:
  Header() = default;
  Header(MetadataFormatFn metadata_and_format_fn, LoggerDetails const* logger_details, uint64_t timestamp)
    : metadata_and_format_fn(metadata_and_format_fn), logger_details(logger_details), timestamp(timestamp)
  {
  }

  MetadataFormatFn metadata_and_format_fn{nullptr};
  LoggerDetails const* logger_details{nullptr};
  uint64_t timestamp{0};
};
} // namespace detail
} // namespace quill
