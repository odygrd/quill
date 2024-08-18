/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/LogLevel.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

QUILL_BEGIN_NAMESPACE

/**
 * Captures and stores information about a logging event in compile time
 */
class MacroMetadata
{
public:
  enum Event : uint8_t
  {
    Log,
    InitBacktrace,
    FlushBacktrace,
    Flush
  };

  constexpr MacroMetadata(char const* source_location, char const* caller_function,
                          char const* message_format, char const* tags, LogLevel log_level, Event event) noexcept
    : _source_location(source_location),
      _caller_function(caller_function),
      _message_format(message_format),
      _tags(tags),
      _colon_separator_pos(_calc_colon_separator_pos()),
      _file_name_pos(_calc_file_name_pos()),
      _log_level(log_level),
      _event(event)
  {
    _set_named_args_flag(_contains_named_args(message_format));
  }

  QUILL_NODISCARD char const* source_location() const noexcept { return _source_location; }

  QUILL_NODISCARD char const* caller_function() const noexcept { return _caller_function; }

  QUILL_NODISCARD char const* message_format() const noexcept { return _message_format; }

  QUILL_NODISCARD char const* line() const noexcept
  {
    return _source_location + _colon_separator_pos + 1;
  }

  QUILL_NODISCARD std::string_view full_path() const noexcept
  {
    return std::string_view{_source_location, _colon_separator_pos};
  }

  QUILL_NODISCARD std::string_view file_name() const noexcept
  {
    return std::string_view{_source_location + _file_name_pos,
                            static_cast<size_t>(_colon_separator_pos - _file_name_pos)};
  }

  QUILL_NODISCARD char const* short_source_location() const noexcept
  {
    return _source_location + _file_name_pos;
  }

  QUILL_NODISCARD LogLevel log_level() const noexcept { return _log_level; }

  QUILL_NODISCARD char const* tags() const noexcept { return _tags; }

  QUILL_NODISCARD bool has_named_args() const noexcept { return _format_flags & NAMED_ARGS_FLAG; }

  QUILL_NODISCARD Event event() const noexcept { return _event; }

private:
  /***/
  QUILL_NODISCARD constexpr uint16_t _calc_file_name_pos() const noexcept
  {
    char const* source_location = _source_location;
    char const* file = source_location;
    while (*source_location)
    {
      char cur = *source_location++;
      if (cur == '/' || cur == PATH_PREFERRED_SEPARATOR)
      {
        file = source_location;
      }
    }
    return static_cast<uint16_t>(file - _source_location);
  }

  /***/
  QUILL_NODISCARD constexpr uint16_t _calc_colon_separator_pos() const noexcept
  {
    std::string_view const source_loc{_source_location};
    auto const separator_index = source_loc.rfind(':');
    return static_cast<uint16_t>(separator_index);
  }

  /***/
  constexpr void _set_named_args_flag(bool value) noexcept
  {
    if (value)
    {
      _format_flags |= NAMED_ARGS_FLAG;
    }
    else
    {
      _format_flags &= static_cast<uint8_t>(~NAMED_ARGS_FLAG);
    }
  }

  /***/
  static constexpr bool _contains_named_args(std::string_view fmt) noexcept
  {
    uint32_t pos{0};
    bool found_named_arg{false};

    // Iterates the format string and checks if any characters are contained inside `{}`
    while (pos < fmt.length())
    {
      if (fmt[pos] == '{')
      {
        ++pos; // consume {
        if (pos >= fmt.length())
        {
          break;
        }

        // first character after the {
        auto const fc = fmt[pos];
        if (fc == '{')
        {
          // this means first '{' was escaped, so we ignore both of them
          ++pos;
          continue;
        }

        // else look for the next '}'
        uint32_t char_cnt{0};
        while (pos < fmt.length())
        {
          if (fmt[pos] == '}')
          {
            ++pos; // consume }
            if (pos >= fmt.length())
            {
              break;
            }

            if (fmt[pos] == '}')
            {
              // this means first '}', was escaped ignore it
              ++pos;
              ++char_cnt;
              continue;
            }

            // we found '{' match, we can break
            break;
          }

          ++pos;
          ++char_cnt;
        }

        if ((char_cnt != 0) && ((fc >= 'a' && fc <= 'z') || (fc >= 'A' && fc <= 'Z')))
        {
          found_named_arg = true;
        }
      }
      ++pos;
    }

    return found_named_arg;
  }

private:
  // define our own PATH_PREFERRED_SEPARATOR to not include <filesystem>
#if defined(_WIN32) && !defined(__CYGWIN__)
  static constexpr wchar_t PATH_PREFERRED_SEPARATOR = L'\\';
#else
  static constexpr char PATH_PREFERRED_SEPARATOR = '/';
#endif

  static constexpr uint8_t NAMED_ARGS_FLAG = 0x01;

  char const* _source_location;
  char const* _caller_function;
  char const* _message_format;
  char const* _tags;
  uint16_t _colon_separator_pos;
  uint16_t _file_name_pos;
  LogLevel _log_level;
  Event _event;
  uint8_t _format_flags{0};
};

static_assert(sizeof(MacroMetadata) <= detail::CACHE_LINE_SIZE,
              "Size of MacroMetadata exceeds the cache line size");

QUILL_END_NAMESPACE