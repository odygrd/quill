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
#include <string_view>
#include <string>
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
QUILL_NODISCARD inline constexpr bool need_call_dtor_for()
{
  using ArgType = detail::remove_cvref_t<Arg>;

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

  return !std::is_trivially_destructible<ArgType>::value;
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
  using ArgType = detail::remove_cvref_t<Arg>;

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
    in = detail::align_pointer<alignof(Arg), std::byte>(in);
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
    in = detail::align_pointer<alignof(Arg), std::byte>(in);
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
    in = detail::align_pointer<alignof(Arg), std::byte>(in);

    args.emplace_back(fmt::detail::make_arg<fmt::format_context>(*reinterpret_cast<ArgType*>(in)));

    if constexpr (need_call_dtor_for<Arg>())
    {
      destruct_args[DestructIdx] = in;
      return decode_args<DestructIdx + 1, Args...>(in + sizeof(ArgType), args, destruct_args);
    }
    else
    {
      return decode_args<DestructIdx, Args...>(in + sizeof(ArgType), args, destruct_args);
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
  using ArgType = detail::remove_cvref_t<Arg>;
  if constexpr (need_call_dtor_for<Arg>())
  {
    (reinterpret_cast<ArgType*>(args[DestructIdx]))->~ArgType();
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
  if constexpr (is_type_of_c_string<Arg>())
  {
    size_t const len = strlen(arg) + 1;
    c_string_sizes[CstringIdx] = len;
    return len + get_args_sizes<CstringIdx + 1>(c_string_sizes, args...);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    // for std::string we also need to store the size in order to correctly retrieve it
    // the reason for this is that if we create e.g:
    // std::string msg = fmt::format("{} {} {} {} {}", (char)0, (char)0, (char)0, (char)0,
    // "sssssssssssssssssssssss"); then strlen(msg.data()) = 0 but msg.size() = 31
    return (arg.size() + sizeof(size_t)) + alignof(size_t) +
      get_args_sizes<CstringIdx>(c_string_sizes, args...);
  }
#if defined(_WIN32)
  else if constexpr (is_type_of_wide_c_string<Arg>())
  {
    size_t const len = get_wide_string_encoding_size(std::wstring_view {arg, wcslen(arg)});
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
  if constexpr (is_type_of_c_string<Arg>())
  {
    std::memcpy(out, arg, c_string_sizes[CstringIdx]);
    return encode_args<CstringIdx + 1>(c_string_sizes, out + c_string_sizes[CstringIdx],
                                       std::forward<Args>(args)...);
  }
  else if constexpr (is_type_of_string<Arg>())
  {
    // for std::string we store the size first, in order to correctly retrieve it
    out = detail::align_pointer<alignof(Arg), std::byte>(out);
    size_t const len = arg.length();
    std::memcpy(out, &len, sizeof(size_t));
    out += sizeof(size_t);

    // copy the string, no need to zero terminate it as we got the length
    std::memcpy(out, arg.data(), arg.length());
    return encode_args<CstringIdx>(c_string_sizes, out + arg.length(), std::forward<Args>(args)...);
  }
#if defined(_WIN32)
  else if constexpr (is_type_of_wide_c_string<Arg>())
  {
    out = detail::align_pointer<alignof(Arg), std::byte>(out);
    size_t const len = c_string_sizes[CstringIdx];
    std::memcpy(out, &len, sizeof(size_t));

    out += sizeof(size_t);
    wide_string_to_narrow(out, c_string_sizes[CstringIdx], std::wstring_view{arg, wcslen(arg)});
    return encode_args<CstringIdx + 1>(c_string_sizes, out + c_string_sizes[CstringIdx],
                                       std::forward<Args>(args)...);
  }
  else if constexpr (is_type_of_wide_string<Arg>())
  {
    // for std::wstring we store the size first, in order to correctly retrieve it
    out = detail::align_pointer<alignof(Arg), std::byte>(out);
    size_t const len = c_string_sizes[CstringIdx];
    std::memcpy(out, &len, sizeof(size_t));
    out += sizeof(size_t);

    wide_string_to_narrow(out, c_string_sizes[CstringIdx], arg);
    return encode_args<CstringIdx + 1>(c_string_sizes, out + c_string_sizes[CstringIdx],
                                       std::forward<Args>(args)...);
  }
#endif
  else
  {
    // no need to align for chars, but align for any other type
    out = detail::align_pointer<alignof(Arg), std::byte>(out);

    // use memcpy when possible
    if constexpr (std::is_trivially_copyable_v<detail::remove_cvref_t<Arg>>)
    {
      std::memcpy(out, &arg, sizeof(Arg));
    }
    else
    {
      new (out) detail::remove_cvref_t<Arg>(std::forward<Arg>(arg));
    }

    return encode_args<CstringIdx>(c_string_sizes, out + sizeof(Arg), std::forward<Args>(args)...);
  }
}

/**
 * Format function
 */
using FormatToFn = std::byte* (*)(fmt::string_view format, std::byte* data, fmt::memory_buffer& out,
                                  std::vector<fmt::basic_format_arg<fmt::format_context>>& args);

template <typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::byte* format_to(fmt::string_view format, std::byte* data,
                                                         fmt::memory_buffer& out,
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
} // namespace detail

/**
 * Stores the source metadata and additionally a span of TypeDescriptors
 */
struct Metadata
{
  /**
   * Creates and/or returns a pointer to Metadata with static lifetime
   * @tparam MacroMetadataFun
   * @tparam Args
   * @return
   */
  template <typename MacroMetadataFun, typename... Args>
  [[nodiscard]] static Metadata const* get() noexcept
  {
    static constexpr Metadata metadata{MacroMetadataFun{}(), detail::format_to<Args...>};
    return std::addressof(metadata);
  }

  constexpr Metadata(MacroMetadata macro_metadata, detail::FormatToFn format_to)
    : macro_metadata(macro_metadata), format_to_fn(format_to)
  {
  }

  MacroMetadata macro_metadata;
  detail::FormatToFn format_to_fn;
};

/**
 * A variable template that will call Metadata::get() during the program initialisation time
 */
template <typename MacroMetadataFun, typename... Args>
Metadata const* get_metadata_ptr{Metadata::get<MacroMetadataFun, Args...>()};

namespace detail
{
struct Header
{
public:
  Header() = default;
  Header(Metadata const* metadata, detail::LoggerDetails const* logger_details, uint64_t timestamp)
    : metadata(metadata), logger_details(logger_details), timestamp(timestamp){};

  Metadata const* metadata{nullptr};
  detail::LoggerDetails const* logger_details{nullptr};
  uint64_t timestamp{0};
};
} // namespace detail
} // namespace quill
