/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"

#include "quill/bundled/fmt/format.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * Backend helpers for format strings with named arguments, e.g. "{name} sent {count:>4}".
 * process_named_args_format_message() turns such a string into a positional fmt string,
 * "{} sent {:>4}", and the list of argument names with their format specs, which the backend
 * uses to fill LogRecord::named_args and JSON sinks.
 */

/** @return `arg_name`, or `arg_name` with a numeric suffix if `existing_keys` already has it */
QUILL_NODISCARD inline std::string make_unique_named_arg_key(
  std::vector<std::pair<std::string, std::string>> const& existing_keys, std::string_view arg_name)
{
  std::string key{arg_name};

  for (size_t suffix = 1;; ++suffix)
  {
    bool key_already_exists = false;
    for (auto const& existing_key : existing_keys)
    {
      if (existing_key.first == key)
      {
        key_already_exists = true;
        break;
      }
    }

    if (!key_already_exists)
    {
      return key;
    }

    key = std::string{arg_name};
    key += "_";
    key += std::to_string(suffix);
  }
}

/** @return the position of the `}` closing the placeholder that starts before `pos`, or npos */
QUILL_NODISCARD inline size_t find_placeholder_closing_brace(std::string_view text, size_t pos) noexcept
{
  size_t nested_replacement_fields = 0;

  while (pos < text.size())
  {
    if (text[pos] == '{')
    {
      if ((pos + 1 < text.size()) && (text[pos + 1] == '{'))
      {
        pos += 2;
        continue;
      }

      ++nested_replacement_fields;
      ++pos;
      continue;
    }

    if (text[pos] == '}')
    {
      if (nested_replacement_fields == 0)
      {
        return pos;
      }

      --nested_replacement_fields;
    }

    ++pos;
  }

  return std::string_view::npos;
}

/**
 * Replaces nested named fields in a format spec, e.g. ":{width}" becomes ":{}".
 * @param dynamic_arg_count incremented for each nested field, as each one consumes an argument
 */
QUILL_NODISCARD inline std::string normalize_named_arg_syntax(std::string_view arg_syntax, size_t& dynamic_arg_count)
{
  std::string normalized;
  normalized.reserve(arg_syntax.size());

  size_t pos = 0;
  while (pos < arg_syntax.size())
  {
    if (arg_syntax[pos] != '{')
    {
      normalized += arg_syntax[pos];
      ++pos;
      continue;
    }

    if ((pos + 1 < arg_syntax.size()) && (arg_syntax[pos + 1] == '{'))
    {
      normalized += "{{";
      pos += 2;
      continue;
    }

    size_t const close_bracket_pos = find_placeholder_closing_brace(arg_syntax, pos + 1);
    if (close_bracket_pos == std::string_view::npos)
    {
      normalized += std::string{arg_syntax.substr(pos)};
      break;
    }

    normalized += "{}";
    ++dynamic_arg_count;
    pos = close_bracket_pos + 1;
  }

  return normalized;
}

/**
 * Process the format of a log message that contains named args
 * @param fmt_template a log message containing named arguments
 * @return first: fmt string without the named arguments, second: a vector extracted keys
 */
QUILL_ATTRIBUTE_HOT inline std::pair<std::string, std::vector<std::pair<std::string, std::string>>> process_named_args_format_message(
  std::string_view fmt_template)
{
  // It would be nice to do this at compile time and store it in macro metadata, but without
  // constexpr vector and string in c++17 it is not possible
  std::string fmt_str;
  std::vector<std::pair<std::string, std::string>> keys;
  fmt_str.reserve(fmt_template.size());

  size_t pos = 0;
  while (pos < fmt_template.size())
  {
    if (fmt_template[pos] != '{')
    {
      fmt_str += fmt_template[pos];
      ++pos;
      continue;
    }

    if ((pos + 1 < fmt_template.size()) && (fmt_template[pos + 1] == '{'))
    {
      fmt_str += "{{";
      pos += 2;
      continue;
    }

    size_t const placeholder_start = pos;
    ++pos; // consume '{'

    size_t const close_bracket_pos = find_placeholder_closing_brace(fmt_template, pos);
    if (close_bracket_pos == std::string_view::npos)
    {
      fmt_str += std::string{fmt_template.substr(placeholder_start)};
      break;
    }

    std::string_view const text_inside_placeholders = fmt_template.substr(pos, close_bracket_pos - pos);
    std::string_view arg_syntax;
    std::string_view arg_name;

    if (size_t const syntax_separator = text_inside_placeholders.find(':');
        syntax_separator != std::string_view::npos)
    {
      arg_syntax = text_inside_placeholders.substr(
        syntax_separator, text_inside_placeholders.size() - syntax_separator);
      arg_name = text_inside_placeholders.substr(0, syntax_separator);
    }
    else
    {
      arg_name = text_inside_placeholders;
    }

    size_t dynamic_arg_count{0};
    std::string const normalized_arg_syntax = normalize_named_arg_syntax(arg_syntax, dynamic_arg_count);

    fmt_str += "{";
    fmt_str += normalized_arg_syntax;
    fmt_str += "}";

    keys.emplace_back(make_unique_named_arg_key(keys, arg_name), normalized_arg_syntax);
    // Width and precision consume argument slots before the next outer field.
    for (size_t i = 0; i < dynamic_arg_count; ++i)
    {
      keys.emplace_back(make_unique_named_arg_key(keys, fmtquill::format("_{}", keys.size())), std::string{});
    }
    pos = close_bracket_pos + 1;
  }

  return std::make_pair(std::move(fmt_str), std::move(keys));
}
} // namespace detail

QUILL_END_NAMESPACE
