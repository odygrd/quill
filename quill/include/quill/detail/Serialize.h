/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "misc/Utilities.h"
#include "quill/LogLevel.h"
#include "quill/MacroMetadata.h"
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

/** Forward declaration **/
class LoggerDetails;

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_c_string()
{
  using arg_t = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<arg_t, char const*>, std::is_same<arg_t, char*>>;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_string()
{
  using arg_t = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<arg_t, std::string>, std::is_same<arg_t, std::string_view>>;
}

#if defined(_WIN32)
template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_wide_c_string()
{
  using arg_t = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<arg_t, wchar_t const*>, std::is_same<arg_t, wchar_t*>>;
}

template <typename Arg>
QUILL_NODISCARD constexpr bool is_type_of_wide_string()
{
  using arg_t = std::decay_t<Arg>;
  return std::disjunction_v<std::is_same<arg_t, std::wstring>, std::is_same<arg_t, std::wstring_view>>;
}
#endif

template <typename Arg>
QUILL_NODISCARD inline constexpr bool need_call_dtor_for()
{
  using arg_t = detail::remove_cvref_t<Arg>;

#if defined(_WIN32)
  if constexpr (is_type_of_wide_string<arg_t>())
  {
    return false;
  }
#endif

  if constexpr (is_type_of_string<arg_t>())
  {
    return false;
  }

  return !std::is_trivially_destructible<arg_t>::value;
}

template <size_t DestructIdx>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::byte* decode_args(
  std::byte* in, std::vector<fmt::basic_format_arg<fmt::format_context>>&, std::byte**)
{
  return in;
}

template <size_t DestructIdx, typename Arg, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::byte* decode_args(
  std::byte* in, std::vector<fmt::basic_format_arg<fmt::format_context>>& args, std::byte** destruct_args)
{
  using arg_t = detail::remove_cvref_t<Arg>;

  if constexpr (is_type_of_c_string<Arg>())
  {
    char const* str = reinterpret_cast<char const*>(in);
    std::string_view const v{str, strlen(str)};
    args.emplace_back(fmt::detail::make_arg<fmt::format_context>(v));
    return decode_args<DestructIdx, Args...>(in + v.length() + 1, args, destruct_args);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    // for std::string we first need to retrieve the length
    in = detail::align_pointer<alignof(size_t), std::byte>(in);
    size_t len{0};
    std::memcpy(&len, in, sizeof(size_t));
    in += sizeof(size_t);

    // retrieve the rest of the string
    char const* str = reinterpret_cast<char const*>(in);
    std::string_view const v{str, len};
    args.emplace_back(fmt::detail::make_arg<fmt::format_context>(v));
    return decode_args<DestructIdx, Args...>(in + v.length(), args, destruct_args);
  }
#if defined(_WIN32)
  else if constexpr (is_type_of_wide_c_string<Arg>() || is_type_of_wide_string<Arg>())
  {
    // for std::wstring we first need to retrieve the length
    in = detail::align_pointer<alignof(size_t), std::byte>(in);
    size_t len{0};
    std::memcpy(&len, in, sizeof(size_t));
    in += sizeof(size_t);

    char const* str = reinterpret_cast<char const*>(in);
    std::string_view const v{str, len};
    args.emplace_back(fmt::detail::make_arg<fmt::format_context>(v));
    return decode_args<DestructIdx, Args...>(in + v.length(), args, destruct_args);
  }
#endif
  else
  {
    // no need to align for chars, but align for any other type
    in = detail::align_pointer<alignof(arg_t), std::byte>(in);

    args.emplace_back(fmt::detail::make_arg<fmt::format_context>(*reinterpret_cast<arg_t*>(in)));

    if constexpr (need_call_dtor_for<Arg>())
    {
      destruct_args[DestructIdx] = in;
      return decode_args<DestructIdx + 1, Args...>(in + sizeof(arg_t), args, destruct_args);
    }
    else
    {
      return decode_args<DestructIdx, Args...>(in + sizeof(arg_t), args, destruct_args);
    }
  }
}

template <size_t DestructIdx>
QUILL_ATTRIBUTE_HOT inline void destruct_args(std::byte**)
{
}

template <size_t DestructIdx, typename Arg, typename... Args>
QUILL_ATTRIBUTE_HOT inline void destruct_args(std::byte** args)
{
  using arg_t = detail::remove_cvref_t<Arg>;

  if constexpr (need_call_dtor_for<Arg>())
  {
    (reinterpret_cast<arg_t*>(args[DestructIdx]))->~arg_t();
    destruct_args<DestructIdx + 1, Args...>(args);
  }
  else
  {
    destruct_args<DestructIdx, Args...>(args);
  }
}

template <typename Arg>
QUILL_ATTRIBUTE_HOT void get_args_size(size_t& result_sum, size_t* c_string_sizes, size_t& c_str_index, Arg&& arg)
{
  using arg_t = detail::remove_cvref_t<Arg>;

  if constexpr (is_type_of_c_string<Arg>())
  {
    size_t const len = strlen(arg) + 1;
    c_string_sizes[c_str_index++] = len;
    result_sum += len;
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    // for std::string we also need to store the size in order to correctly retrieve it
    // the reason for this is that if we create e.g:
    // std::string msg = fmt::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
    // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
    result_sum += arg.size() + sizeof(size_t) + alignof(size_t);
  }
#if defined(_WIN32)
  else if constexpr (is_type_of_wide_c_string<Arg>())
  {
    size_t const len = get_wide_string_encoding_size(std::wstring_view{arg, wcslen(arg)});
    c_string_sizes[c_str_index++] = len;
    result_sum += len + sizeof(size_t) + alignof(size_t);
  }
  else if constexpr (is_type_of_wide_string<Arg>())
  {
    size_t const len = get_wide_string_encoding_size(arg);
    c_string_sizes[c_str_index++] = len;
    result_sum += len + sizeof(size_t) + alignof(size_t);
  }
#endif
  else
  {
    result_sum += alignof(arg_t) + sizeof(arg_t);
  }
}

/**
 * Get the size of all arguments
 */
template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT size_t get_args_sizes(size_t* c_string_sizes, Args&&... args)
{
  if constexpr (sizeof...(args) != 0)
  {
    size_t c_string_index{0};
    size_t result{0};

    // we use ',' operator here instead of '+' to guarantee it will be evaluated from left-to-right
    (get_args_size(result, c_string_sizes, c_string_index, std::forward<Args>(args)), ...);

    return result;
  }
  else
  {
    // no variadic args
    return 0;
  }
}

/**
 * Encode args to the buffer
 */
template <typename Arg>
QUILL_ATTRIBUTE_HOT constexpr void encode_arg(std::byte*& out, size_t const* c_string_sizes,
                                              size_t& c_str_index, Arg&& arg)
{
  using arg_t = detail::remove_cvref_t<Arg>;

  if constexpr (is_type_of_c_string<arg_t>())
  {
    std::memcpy(out, arg, c_string_sizes[c_str_index]);
    out += c_string_sizes[c_str_index++];
  }
  else if constexpr (is_type_of_string<arg_t>())
  {
    // for std::string we store the size first, in order to correctly retrieve it
    out = detail::align_pointer<alignof(size_t), std::byte>(out);
    size_t const len = arg.length();
    std::memcpy(out, &len, sizeof(size_t));
    out += sizeof(size_t);

    // copy the string, no need to zero terminate it as we got the length
    std::memcpy(out, arg.data(), arg.length());
    out += arg.length();
  }
#if defined(_WIN32)
  else if constexpr (is_type_of_wide_c_string<arg_t>())
  {
    out = detail::align_pointer<alignof(size_t), std::byte>(out);
    size_t const len = c_string_sizes[c_str_index];
    std::memcpy(out, &len, sizeof(size_t));

    out += sizeof(size_t);
    wide_string_to_narrow(out, c_string_sizes[c_str_index], std::wstring_view{arg, wcslen(arg)});
    out += c_string_sizes[c_str_index++];
  }
  else if constexpr (is_type_of_wide_string<arg_t>())
  {
    // for std::wstring we store the size first, in order to correctly retrieve it
    out = detail::align_pointer<alignof(size_t), std::byte>(out);
    size_t const len = c_string_sizes[c_str_index];
    std::memcpy(out, &len, sizeof(size_t));
    out += sizeof(size_t);

    wide_string_to_narrow(out, c_string_sizes[c_str_index], arg);
    out += c_string_sizes[c_str_index++];
  }
#endif
  else
  {
    // no need to align for chars, but align for any other type
    out = detail::align_pointer<alignof(arg_t), std::byte>(out);

    // use memcpy when possible
    if constexpr (std::is_trivially_copyable_v<arg_t>)
    {
      std::memcpy(out, &arg, sizeof(arg_t));
    }
    else
    {
      new (out) arg_t(std::forward<Arg>(arg));
    }

    out += sizeof(arg_t);
  }
}

template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* encode_args(size_t const* c_string_sizes,
                                                           std::byte* out, Args&&... args)
{
  size_t c_string_index{0};
  (encode_arg(out, c_string_sizes, c_string_index, std::forward<Args>(args)), ...);
  return out;
}

/**
 * Format function
 */
using FormatToFn = std::byte* (*)(std::string_view format, std::byte* data, fmt_buffer_t& out,
                                  std::vector<fmt::basic_format_arg<fmt::format_context>>& args);

template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::byte* format_to(
  std::string_view format, std::byte* data, fmt_buffer_t& out,
  std::vector<fmt::basic_format_arg<fmt::format_context>>& args)
{
  constexpr size_t num_dtors = fmt::detail::count<need_call_dtor_for<Args>()...>();
  std::byte* dtor_args[(std::max)(num_dtors, (size_t)1)];

  args.clear();
  std::byte* ret = decode_args<0, Args...>(data, args, dtor_args);

  out.clear();
  fmt::vformat_to(std::back_inserter(out), format, fmt::basic_format_args(args.data(), sizeof...(Args)));

  destruct_args<0, Args...>(dtor_args);

  return ret;
}

/**
 * This function pointer is used to store and pass the template parameters to the backend worker
 * thread
 */
using MetadataFormatFn = std::pair<MacroMetadata, detail::FormatToFn> (*)();

template <typename TAnonymousStruct, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT constexpr std::pair<MacroMetadata, detail::FormatToFn> get_metadata_and_format_fn()
{
  constexpr auto ret = std::make_pair(TAnonymousStruct{}(), detail::format_to<Args...>);
  return ret;
}

} // namespace detail

namespace detail
{
struct Header
{
public:
  Header() = default;
  Header(MetadataFormatFn metadata_and_format_fn, detail::LoggerDetails const* logger_details, uint64_t timestamp)
    : metadata_and_format_fn(metadata_and_format_fn), logger_details(logger_details), timestamp(timestamp){};

  MetadataFormatFn metadata_and_format_fn{nullptr};
  detail::LoggerDetails const* logger_details{nullptr};
  uint64_t timestamp{0};
};
} // namespace detail
} // namespace quill
