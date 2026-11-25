/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/NamedArgsTemplate.h"
#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

#ifndef QUILL_MODULE
  #include <cstring>
  #include <string>
  #include <unordered_map>
  #include <utility>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * The NamedArgsTemplate of each log statement with named arguments, rewritten once.
 *
 * Entries are keyed by the address of the format string and checked against a copy of its text,
 * so an address that later holds another string, e.g. after a shared library is unloaded and
 * another one is loaded, is rewritten.
 */
class NamedArgsTemplateCache
{
public:
  /**
   * @return the template of `format`
   * @param format the nul-terminated format string of a log statement, which stays at its address
   */
  QUILL_NODISCARD NamedArgsTemplate const& get(char const* format)
  {
    Entry& entry = _entries[format];

    if (QUILL_UNLIKELY(std::strcmp(entry.text.c_str(), format) != 0) || QUILL_UNLIKELY(!entry.is_prepared))
    {
      // A new format string, or its address now holds a different one. Rewrite it first, so an
      // exception leaves the entry as it was
      NamedArgsTemplate named_args_template;
      named_args_template.prepare(format);
      entry.text = format;
      entry.named_args_template = std::move(named_args_template);
      entry.is_prepared = true;
    }

    return entry.named_args_template;
  }

  /**
   * Removes all entries and releases their memory. Releasing it is best effort: an empty map can
   * allocate, e.g. with MSVC, and without memory for it the entries are only removed
   */
  void clear() noexcept
  {
    _entries.clear();

    QUILL_TRY { decltype(_entries){}.swap(_entries); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL() {}
#endif
  }

private:
  struct Entry
  {
    std::string text; /**< The format string, to check that the address still holds it */
    NamedArgsTemplate named_args_template;
    bool is_prepared{false};
  };

  std::unordered_map<char const*, Entry> _entries;
};
} // namespace detail

QUILL_END_NAMESPACE
