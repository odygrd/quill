/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/LogLevel.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"

#include <cassert>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace quill
{
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

#if defined(_WIN32)
  constexpr MacroMetadata(char const* source_location, char const* caller_function,
                          wchar_t const* message_format, CustomTags const* custom_tags, LogLevel log_level,
                          Event event, bool is_structured_log, bool is_printf_format) noexcept
    : _source_location(source_location),
      _caller_function(caller_function),
      _message_format(message_format),
      _custom_tags(custom_tags),
      _colon_separator_pos(_calc_colon_separator_pos()),
      _file_name_pos(_calc_file_name_pos()),
      _log_level(log_level),
      _event(event)
  {
    _set_structured_log_template_flag(is_structured_log);
    _set_printf_format_flag(is_printf_format);
  }
#endif

  constexpr MacroMetadata(char const* source_location, char const* caller_function,
                          char const* message_format, CustomTags const* custom_tags, LogLevel log_level,
                          Event event, bool is_structured_log, bool is_printf_format) noexcept
    : _source_location(source_location),
      _caller_function(caller_function),
      _message_format(message_format),
      _custom_tags(custom_tags),
      _colon_separator_pos(_calc_colon_separator_pos()),
      _file_name_pos(_calc_file_name_pos()),
      _log_level(log_level),
      _event(event)
  {
    _set_structured_log_template_flag(is_structured_log);
    _set_printf_format_flag(is_printf_format);
  }

  QUILL_NODISCARD char const* source_location() const noexcept { return _source_location; }

  QUILL_NODISCARD char const* caller_function() const noexcept { return _caller_function; }

  QUILL_NODISCARD char const* message_format() const noexcept
  {
    assert(!is_wide_char_format());
    return static_cast<char const*>(_message_format);
  }

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

  QUILL_NODISCARD constexpr LogLevel log_level() const noexcept { return _log_level; }

  QUILL_NODISCARD std::string_view log_level_string() const noexcept
  {
    return quill::loglevel_to_string(_log_level);
  }

  QUILL_NODISCARD std::string_view log_level_id() const noexcept
  {
    return quill::loglevel_to_string_id(_log_level);
  }

  QUILL_NODISCARD CustomTags const* custom_tags() const noexcept { return _custom_tags; }

  QUILL_NODISCARD constexpr bool is_structured_log_template() const noexcept
  {
    return _format_flags & STRUCTURED_LOG_TEMPLATE_FLAG;
  }

  QUILL_NODISCARD constexpr bool is_printf_format() const noexcept
  {
    return _format_flags & PRINTF_FORMAT_FLAG;
  }

  QUILL_NODISCARD Event event() const noexcept { return _event; }

  QUILL_NODISCARD constexpr bool is_wide_char_format() const noexcept
  {
    return _format_flags & WIDE_CHAR_FORMAT_FLAG;
  }

#if defined(_WIN32)
  QUILL_NODISCARD wchar_t const* wmessage_format() const noexcept
  {
    assert(is_wide_char_format());
    return static_cast<wchar_t const*>(_message_format);
  }
#endif

private:
  QUILL_NODISCARD constexpr uint16_t _calc_file_name_pos() const noexcept
  {
    char const* source_location = _source_location;
    char const* file = source_location;
    while (*source_location)
    {
      char cur = *source_location++;
      if (cur == '/' || cur == std::filesystem::path::preferred_separator)
      {
        file = source_location;
      }
    }
    return static_cast<uint16_t>(file - _source_location);
  }

  QUILL_NODISCARD constexpr uint16_t _calc_colon_separator_pos() const noexcept
  {
    std::string_view source_loc{_source_location};
    auto const separator_index = source_loc.rfind(':');
    return static_cast<uint16_t>(separator_index);
  }

  constexpr void _set_structured_log_template_flag(bool value) noexcept
  {
    if (value)
    {
      _format_flags |= STRUCTURED_LOG_TEMPLATE_FLAG;
    }
    else
    {
      _format_flags &= ~STRUCTURED_LOG_TEMPLATE_FLAG;
    }
  }

  constexpr void _set_printf_format_flag(bool value) noexcept
  {
    if (value)
    {
      _format_flags |= PRINTF_FORMAT_FLAG;
    }
    else
    {
      _format_flags &= ~PRINTF_FORMAT_FLAG;
    }
  }

  constexpr void _set_wide_char_format_flag(bool value) noexcept
  {
    if (value)
    {
      _format_flags |= WIDE_CHAR_FORMAT_FLAG;
    }
    else
    {
      _format_flags &= ~WIDE_CHAR_FORMAT_FLAG;
    }
  }

private:
  static constexpr uint8_t STRUCTURED_LOG_TEMPLATE_FLAG = 0x01;
  static constexpr uint8_t PRINTF_FORMAT_FLAG = 0x02;
  static constexpr uint8_t WIDE_CHAR_FORMAT_FLAG = 0x04;

  char const* _source_location;
  char const* _caller_function;
  void const* _message_format;
  CustomTags const* _custom_tags;
  uint16_t _colon_separator_pos;
  uint16_t _file_name_pos;
  LogLevel _log_level;
  Event _event;
  uint8_t _format_flags{0};
};

static_assert(sizeof(MacroMetadata) <= detail::CACHE_LINE_SIZE,
              "Size of MacroMetadata exceeds the cache line size");
} // namespace quill