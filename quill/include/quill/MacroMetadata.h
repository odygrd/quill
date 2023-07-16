/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/LogLevel.h"
#include "quill/detail/misc/Common.h"
#include <array>
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

  constexpr MacroMetadata(std::string_view lineno, std::string_view pathname, std::string_view fileline,
                          std::string_view func, std::string_view message_format, LogLevel level,
                          Event event, bool is_structured_log_template, bool is_printf_format)
    : _func(func),
      _pathname(pathname),
      _filename(_extract_source_file_name(_pathname)),
      _fileline(_extract_source_file_name(fileline)),
      _message_format(message_format),
      _lineno(lineno),
      _level(level),
      _event(event),
      _is_structured_log_template(is_structured_log_template),
      _is_printf_format(is_printf_format)
  {
  }

#if defined(_WIN32)
  constexpr MacroMetadata(std::string_view lineno, std::string_view pathname, std::string_view fileline,
                          std::string_view func, std::wstring_view message_format, LogLevel level,
                          Event event, bool is_structured_log_template, bool is_printf_format)
    : _func(func),
      _pathname(pathname),
      _filename(_extract_source_file_name(_pathname)),
      _fileline(_extract_source_file_name(fileline)),
      _lineno(lineno),
      _level(level),
      _event(event),
      _is_structured_log_template(is_structured_log_template),
      _is_printf_format(is_printf_format),
      _has_wide_char{true},
      _wmessage_format(message_format)
  {
  }
#endif

  /**
   * @return The function name
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::string_view func() const noexcept
  {
    return _func;
  }

  /**
   * @return The full pathname of the source file where the logging call was made.
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::string_view pathname() const noexcept
  {
    return _pathname;
  }

  /**
   * @return Short portion of the path name
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::string_view filename() const noexcept
  {
    return _filename;
  }

    /**
   * @return file:line
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::string_view fileline() const noexcept
  {
    return _fileline;

  }
  /**
   * @return The user provided format
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::string_view message_format() const noexcept
  {
    return _message_format;
  }

  /**
   * @return The line number
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::string_view lineno() const noexcept
  {
    return _lineno;
  }

  /**
   * @return The log level of this logging event as an enum
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr LogLevel level() const noexcept { return _level; }

  /**
   * @return  The log level of this logging event as a string
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view level_id_as_str() const noexcept
  {
    return loglevel_to_string_id(_level);
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr Event event() const noexcept { return _event; }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr bool is_structured_log_template() const noexcept
  {
    return _is_structured_log_template;
  }

  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr bool is_printf_format() const noexcept
  {
    return _is_printf_format;
  }

#if defined(_WIN32)
  /**
   * @return true if the user provided a wide char format string
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr bool has_wide_char() const noexcept
  {
    return _has_wide_char;
  }

  /**
   * @return The user provided wide character format
   */
  QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr std::wstring_view wmessage_format() const noexcept
  {
    return _wmessage_format;
  }
#endif

private:
  QUILL_NODISCARD_ALWAYS_INLINE_HOT static constexpr std::string_view _extract_source_file_name(std::string_view pathname) noexcept
  {
    char const* path = pathname.data();
    char const* file = path;
    while (*path)
    {
      char cur = *path++;
      if (cur == '/' || cur == fs::path::preferred_separator)
      {
        file = path;
      }
    }
    return file;
  }

private:
  std::string_view _func;
  std::string_view _pathname;
  std::string_view _filename;
  std::string_view _fileline;
  std::string_view _message_format;
  std::string_view _lineno;
  LogLevel _level{LogLevel::None};
  Event _event{Event::Log};
  bool _is_structured_log_template{false};
  bool _is_printf_format{false};

#if defined(_WIN32)
  bool _has_wide_char{false};
  std::wstring_view _wmessage_format;
#endif
};
} // namespace quill
