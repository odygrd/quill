/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"

#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/std.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

QUILL_BEGIN_NAMESPACE

namespace detail
{
class DecodedErrorCode
{
public:
  DecodedErrorCode(int value, std::string category_name, std::string message)
    : _value(value),
      _category_name(std::move(category_name)),
      _message(std::move(message))
  {
  }

  QUILL_NODISCARD int value() const noexcept { return _value; }
  QUILL_NODISCARD std::string const& category_name() const noexcept { return _category_name; }
  QUILL_NODISCARD std::string const& message() const noexcept { return _message; }

private:
  int _value;
  std::string _category_name;
  std::string _message;
};

inline bool operator==(DecodedErrorCode const& lhs, DecodedErrorCode const& rhs) noexcept
{
  return (lhs.value() == rhs.value()) && (lhs.category_name() == rhs.category_name()) &&
    (lhs.message() == rhs.message());
}

inline bool operator<(DecodedErrorCode const& lhs, DecodedErrorCode const& rhs) noexcept
{
  if (lhs.category_name() != rhs.category_name())
  {
    return lhs.category_name() < rhs.category_name();
  }

  if (lhs.value() != rhs.value())
  {
    return lhs.value() < rhs.value();
  }

  return lhs.message() < rhs.message();
}
} // namespace detail

QUILL_BEGIN_EXPORT

template <>
struct Codec<std::error_code>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache, std::error_code const& arg)
  {
    size_t total_size = Codec<int>::compute_encoded_size(conditional_arg_size_cache, arg.value());

    char const* category_name = arg.category().name();
    std::string const message = arg.message();

    total_size += Codec<std::string_view>::compute_encoded_size(
      conditional_arg_size_cache, std::string_view{category_name ? category_name : ""});
    total_size += Codec<std::string>::compute_encoded_size(conditional_arg_size_cache, message);

    return total_size;
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::error_code const& arg)
  {
    Codec<int>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.value());

    char const* category_name = arg.category().name();
    std::string const message = arg.message();

    Codec<std::string_view>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                                    std::string_view{category_name ? category_name : ""});
    Codec<std::string>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, message);
  }

  static detail::DecodedErrorCode decode_arg(std::byte*& buffer)
  {
    int const value = Codec<int>::decode_arg(buffer);
    std::string category_name{Codec<std::string_view>::decode_arg(buffer)};
    std::string message{Codec<std::string_view>::decode_arg(buffer)};
    return detail::DecodedErrorCode{value, std::move(category_name), std::move(message)};
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};
QUILL_END_EXPORT

QUILL_END_NAMESPACE

template <>
struct fmtquill::formatter<quill::detail::DecodedErrorCode>
{
private:
  fmtquill::format_specs _specs;
  fmtquill::detail::arg_ref<char> _width_ref;
  bool _debug{false};

public:
  FMTQUILL_CONSTEXPR void set_debug_format(bool set = true) { _debug = set; }

  FMTQUILL_CONSTEXPR auto parse(fmtquill::parse_context<>& ctx) -> char const*
  {
    auto it = ctx.begin();
    auto const end = ctx.end();
    if (it == end)
    {
      return it;
    }

    it = fmtquill::detail::parse_align(it, end, _specs);
    if (it == end)
    {
      return it;
    }

    char const c = *it;
    if ((it != end) && (((c >= '0') && (c <= '9')) || (c == '{')))
    {
      it = fmtquill::detail::parse_width(it, end, _specs, _width_ref, ctx);
    }

    if ((it != end) && (*it == '?'))
    {
      _debug = true;
      ++it;
    }

    if ((it != end) && (*it == 's'))
    {
      _specs.set_type(fmtquill::presentation_type::string);
      ++it;
    }

    return it;
  }

  template <typename FormatContext>
  FMTQUILL_CONSTEXPR20 auto format(quill::detail::DecodedErrorCode const& error_code,
                                   FormatContext& ctx) const -> decltype(ctx.out())
  {
    auto specs = _specs;
    fmtquill::detail::handle_dynamic_spec(specs.dynamic_width(), specs.width, _width_ref, ctx);

    auto buf = fmtquill::memory_buffer();
    if (_specs.type() == fmtquill::presentation_type::string)
    {
      buf.append(error_code.message());
    }
    else
    {
      buf.append(fmtquill::string_view{error_code.category_name().data(), error_code.category_name().size()});
      buf.push_back(':');
      fmtquill::detail::write<char>(fmtquill::appender(buf), error_code.value());
    }

    auto quoted = fmtquill::memory_buffer();
    auto str = fmtquill::string_view{buf.data(), buf.size()};
    if (_debug)
    {
      fmtquill::detail::write_escaped_string<char>(std::back_inserter(quoted), str);
      str = fmtquill::string_view{quoted.data(), quoted.size()};
    }

    return fmtquill::detail::write<char>(ctx.out(), str, specs);
  }
};
