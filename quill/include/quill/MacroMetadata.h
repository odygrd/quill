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

  constexpr MacroMetadata(const char* lineno, std::string_view pathname, std::string_view func,
                          std::string_view message_format, LogLevel level, Event event)
    : _func(func),
      _pathname(pathname),
      _filename(_extract_source_file_name(_pathname)),
      _message_format(message_format),
      _lineno(lineno),
      _level(level),
      _event(event)
  {
  }

#if defined(_WIN32)
  constexpr MacroMetadata(const char* lineno, std::string_view pathname, std::string_view func,
                          std::wstring_view message_format, LogLevel level, Event event)
    : _func(func),
      _pathname(pathname),
      _filename(_extract_source_file_name(_pathname)),
      _lineno(lineno),
      _level(level),
      _event(event),
      _has_wide_char{true},
      _wmessage_format(message_format)
  {
  }
#endif

  /**
   * @return The function name
   */
  QUILL_NODISCARD constexpr std::string_view func() const noexcept { return _func; }

  /**
   * @return The full pathname of the source file where the logging call was made.
   */
  QUILL_NODISCARD constexpr std::string_view pathname() const noexcept { return _pathname; }

  /**
   * @return Short portion of the path name
   */
  QUILL_NODISCARD constexpr std::string_view filename() const noexcept { return _filename; }

  /**
   * @return The user provided format
   */
  QUILL_NODISCARD constexpr std::string_view message_format() const noexcept
  {
    return _message_format;
  }

  /**
   * @return The line number
   */
  QUILL_NODISCARD constexpr std::string_view lineno() const noexcept { return _lineno; }

  /**
   * @return The log level of this logging event as an enum
   */
  QUILL_NODISCARD constexpr LogLevel level() const noexcept { return _level; }

  /**
   * @return  The log level of this logging event as a string
   */
  QUILL_NODISCARD constexpr std::string_view level_as_str() const noexcept
  {
    return _log_level_to_string(_level);
  }

  /**
   * @return  The log level of this logging event as a string
   */
  QUILL_NODISCARD constexpr std::string_view level_id_as_str() const noexcept
  {
    return _log_level_id_to_string(_level);
  }

  QUILL_NODISCARD constexpr Event event() const noexcept { return _event; }

#if defined(_WIN32)
  /**
   * @return true if the user provided a wide char format string
   */
  QUILL_NODISCARD constexpr bool has_wide_char() const noexcept { return _has_wide_char; }

  /**
   * @return The user provided wide character format
   */
  QUILL_NODISCARD constexpr std::wstring_view wmessage_format() const noexcept
  {
    return _wmessage_format;
  }
#endif

private:
  QUILL_NODISCARD static constexpr std::string_view _extract_source_file_name(std::string_view pathname) noexcept
  {
    char const* path = pathname.data();
    char const* file = path;
    while (*path)
    {
      if (*path++ == fs::path::preferred_separator)
      {
        file = path;
      }
    }
    return file;  }

  QUILL_NODISCARD static constexpr std::string_view _log_level_to_string(LogLevel log_level)
  {
    constexpr std::array<std::string_view, 10> log_levels_strings = {
      {"TRACE_L3 ", "TRACE_L2 ", "TRACE_L1 ", "DEBUG    ", "INFO     ", "WARNING  ", "ERROR    ",
       "CRITICAL ", "BACKTRACE", "NONE"}};

    using log_lvl_t = std::underlying_type<LogLevel>::type;
    return log_levels_strings[static_cast<log_lvl_t>(log_level)];
  }

  QUILL_NODISCARD static constexpr std::string_view _log_level_id_to_string(LogLevel log_level)
  {
    constexpr std::array<std::string_view, 10> log_levels_strings = {
      {"T3", "T2", "T1", "D", "I", "W", "E", "C", "BT", "N"}};

    using log_lvl_t = std::underlying_type<LogLevel>::type;
    return log_levels_strings[static_cast<log_lvl_t>(log_level)];
  }

private:
  std::string_view _func;
  std::string_view _pathname;
  std::string_view _filename;
  std::string_view _message_format;
  std::string_view _lineno;
  LogLevel _level{LogLevel::None};
  Event _event{Event::Log};

#if defined(_WIN32)
  bool _has_wide_char{false};
  std::wstring_view _wmessage_format;
#endif
};
} // namespace quill
