/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/NamedArgsFormat.h"
#include "quill/core/Attributes.h"

#ifndef QUILL_MODULE
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
 * rewritten for the backend: the positional fmt string "{} paid {:.2f}" and the name and format
 * spec of each argument.
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
  }

  /** @return the format string with positional fields, e.g. "{} paid {:.2f}" */
  QUILL_NODISCARD std::string const& format() const noexcept { return _format; }

  /** @return the name and format spec of each named argument */
  QUILL_NODISCARD std::vector<std::pair<std::string, std::string>> const& arg_names() const noexcept
  {
    return _arg_names;
  }

private:
  std::string _format; /**< The format string, with positional fields */
  std::vector<std::pair<std::string, std::string>> _arg_names;
};
} // namespace detail

QUILL_END_NAMESPACE
