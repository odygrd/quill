/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

QUILL_BEGIN_NAMESPACE

namespace detail
{
class BackendMdcState
{
public:
  explicit BackendMdcState(std::string const& pattern) : _format_parts(pattern) {}

  void set(std::string_view key, std::string_view value)
  {
    _fields[std::string{key}] = std::string{value};
  }

  void erase(std::string_view key)
  {
    _fields.erase(std::string{key});
  }

  void clear()
  {
    _fields.clear();
    _formatted_mdc.clear();
  }

  QUILL_NODISCARD std::string_view formatted_mdc() const noexcept { return _formatted_mdc; }

  QUILL_NODISCARD bool empty() const noexcept { return _fields.empty(); }

  void rebuild_formatted_mdc()
  {
    _formatted_mdc.clear();

    if (_fields.empty())
    {
      return;
    }

    _formatted_mdc.append(_format_parts.prefix);

    size_t i = 0;
    for (auto const& [key, value] : _fields)
    {
      _formatted_mdc.append(key);
      _formatted_mdc.append(_format_parts.kv_sep);
      _formatted_mdc.append(value);

      if (++i != _fields.size())
      {
        _formatted_mdc.append(_format_parts.field_sep);
      }
    }

    _formatted_mdc.append(_format_parts.suffix);
  }

private:
  struct FormatParts
  {
    std::string prefix;
    std::string kv_sep;
    std::string field_sep;
    std::string suffix;

    explicit FormatParts(std::string const& pattern)
    {
      if (!_set_from_pattern(pattern))
      {
        QUILL_THROW(QuillError{
          "Invalid BackendOptions::mdc_format_pattern. Expected exactly two \"{}\" placeholders "
          "and at least one trailing character after the second placeholder."});
      }
    }

  private:
    bool _set_from_pattern(std::string_view pattern)
    {
      static constexpr std::string_view placeholder{"{}"};

      size_t const first = pattern.find(placeholder);
      if (first == std::string_view::npos)
      {
        return false;
      }

      size_t const second = pattern.find(placeholder, first + placeholder.size());
      if (second == std::string_view::npos)
      {
        return false;
      }

      size_t const third = pattern.find(placeholder, second + placeholder.size());
      if (third != std::string_view::npos)
      {
        return false;
      }

      size_t const after_second = second + placeholder.size();
      if (after_second >= pattern.size())
      {
        return false;
      }

      prefix.assign(pattern.data(), first);
      kv_sep.assign(pattern.data() + first + placeholder.size(),
                    second - first - placeholder.size());
      field_sep.assign(pattern.data() + after_second, pattern.size() - after_second - 1u);
      suffix.assign(pattern.data() + pattern.size() - 1u, 1u);
      return true;
    }
  };

  FormatParts _format_parts;
  std::map<std::string, std::string> _fields;
  std::string _formatted_mdc;
};
} // namespace detail

QUILL_END_NAMESPACE
