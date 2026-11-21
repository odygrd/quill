/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/NamedArgsFormat.h"
#include "quill/core/Attributes.h"
#include "quill/core/BufferUtilities.h"
#include "quill/core/Common.h"

#include "quill/bundled/fmt/base.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * The format string of a log statement with named arguments, e.g. "{name} paid {price:.2f}",
 * rewritten for the backend: the positional fmt string "{} paid {:.2f}", the name and format spec
 * of each argument, and the literal text around its fields.
 *
 * The backend formats each argument once, into the value of its named argument, see
 * LogRecord::named_args. build_from_values() builds the message from these values and the literal
 * text, so that the arguments are not formatted a second time for the message.
 */
class NamedArgsTemplate
{
public:
  /**
   * Rewrites `format`, reusing the memory of the format string rewritten before
   * @param format a format string with named arguments
   */
  void prepare(std::string_view format)
  {
    auto [positional_format, arg_names] = process_named_args_format_message(format);
    _format = std::move(positional_format);
    _arg_names = std::move(arg_names);

    // Until the fields are parsed, e.g. if parsing throws, the message is left to fmt
    _can_build_from_values = false;
    _segments.clear();
    _can_build_from_values = _parse_segments();
  }

  /**
   * @return true if build_from_values() can build the message: each field of the format string
   * formats the next argument with exactly the format spec of its named argument. Other syntax,
   * e.g. a dynamic width or an unmatched brace, is left to fmt, which also reports its errors
   */
  QUILL_NODISCARD bool can_build_from_values() const noexcept { return _can_build_from_values; }

  /**
   * Appends the format string with each field replaced by the value of its named argument
   * @param values the name and value of each named argument, in the order of arg_names(), each
   * value formatted with the format spec there
   */
  void build_from_values(fmtquill::detail::buffer<char>& out,
                         std::vector<std::pair<std::string, std::string>> const& values) const
  {
    QUILL_ASSERT(_can_build_from_values && (values.size() >= _arg_names.size()),
                 "build_from_values() requires a value for each named argument");

    char const* const text = _format.data();
    size_t value_index{0};

    for (Segment const& segment : _segments)
    {
      append_to_buffer(
        out, std::string_view{text + segment.literal_begin, segment.literal_end - segment.literal_begin});

      if (segment.has_field)
      {
        append_to_buffer(out, values[value_index].second);
        ++value_index;
      }
    }
  }

  /** @return the format string with positional fields, e.g. "{} paid {:.2f}" */
  QUILL_NODISCARD std::string const& format() const noexcept { return _format; }

  /** @return the name and format spec of each named argument */
  QUILL_NODISCARD std::vector<std::pair<std::string, std::string>> const& arg_names() const noexcept
  {
    return _arg_names;
  }

private:
  /** Literal text of _format, followed by a field if has_field */
  struct Segment
  {
    uint32_t literal_begin;
    uint32_t literal_end;
    bool has_field;
  };

  /** @return false if _format has syntax that is left to fmt */
  QUILL_NODISCARD bool _parse_segments()
  {
    std::string_view const format{_format};

    if (format.size() >= UINT32_MAX)
    {
      return false;
    }

    uint32_t literal_begin{0};
    uint32_t pos{0};
    size_t field_count{0};

    while (pos < format.size())
    {
      char const c = format[pos];

      if ((c != '{') && (c != '}'))
      {
        ++pos;
        continue;
      }

      bool const is_escaped_brace = ((pos + 1u) < format.size()) && (format[pos + 1u] == c);

      if (is_escaped_brace)
      {
        // Keep one brace of the pair as literal text and skip the other
        _segments.push_back(Segment{literal_begin, pos + 1u, false});
        pos += 2u;
        literal_begin = pos;
        continue;
      }

      if ((c == '}') || (field_count == _arg_names.size()))
      {
        // an unmatched '}', or a field without a named argument
        return false;
      }

      // The value of a named argument is formatted with "{" + its format spec + "}", so its field
      // must be exactly that text. A format spec with a brace is a nested field, such as a dynamic
      // width, or malformed.
      std::string_view const format_spec{_arg_names[field_count].second};
      size_t const field_size = format_spec.size() + 2u;
      bool const is_field = (format_spec.find_first_of("{}") == std::string_view::npos) &&
        ((format.size() - pos) >= field_size) &&
        (format.compare(pos + 1u, format_spec.size(), format_spec) == 0) &&
        (format[pos + field_size - 1u] == '}');

      if (!is_field)
      {
        return false;
      }

      _segments.push_back(Segment{literal_begin, pos, true});
      ++field_count;
      pos += static_cast<uint32_t>(field_size);
      literal_begin = pos;
    }

    if (literal_begin != pos)
    {
      _segments.push_back(Segment{literal_begin, pos, false});
    }

    // Each named argument has its field
    return field_count == _arg_names.size();
  }

  std::string _format; /**< The format string, with positional fields */
  std::vector<std::pair<std::string, std::string>> _arg_names;
  std::vector<Segment> _segments;
  bool _can_build_from_values{false};
};
} // namespace detail

QUILL_END_NAMESPACE
