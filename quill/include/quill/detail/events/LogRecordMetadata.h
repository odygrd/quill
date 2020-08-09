/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/LogLevel.h"
#include "quill/detail/misc/Common.h"
#include <array>
#include <cstdint>

namespace quill
{
namespace detail
{

/**
 * Captures and stores information about a logging event in compile time
 * This information is later passed to the LogRecordEvent runtime class
 */
class LogRecordMetadata
{
public:
  constexpr LogRecordMetadata() = default;

  constexpr LogRecordMetadata(const char* lineno, char const* pathname, char const* func,
                              char const* message_format, LogLevel level)
    : _func(func),
      _pathname(pathname),
      _filename(_extract_source_file_name(_pathname)),
      _message_format(message_format),
      _lineno(lineno),
      _level(level)
  {
  }

  /**
   * @return The function name
   */
  QUILL_NODISCARD constexpr char const* func() const noexcept { return _func; }

  /**
   * @return The full pathname of the source file where the logging call was made.
   */
  QUILL_NODISCARD constexpr char const* pathname() const noexcept { return _pathname; }

  /**
   * @return Short portion of the path name
   */
  QUILL_NODISCARD constexpr char const* filename() const noexcept { return _filename; }

  /**
   * @return The user provided format
   */
  QUILL_NODISCARD constexpr char const* message_format() const noexcept { return _message_format; }

  /**
   * @return The line number
   */
  QUILL_NODISCARD constexpr char const* lineno() const noexcept { return _lineno; }

  /**
   * @return The log level of this logging event as an enum
   */
  QUILL_NODISCARD constexpr LogLevel level() const noexcept { return _level; }

  /**
   * @return  The log level of this logging event as a string
   */
  QUILL_NODISCARD constexpr char const* level_as_str() const noexcept
  {
    return _log_level_to_string(_level);
  }

private:
  QUILL_NODISCARD static constexpr char const* _str_end(char const* str) noexcept
  {
    return *str ? _str_end(str + 1) : str;
  }

  QUILL_NODISCARD static constexpr bool _str_slant(char const* str) noexcept
  {
    return *str == path_delimiter ? true : (*str ? _str_slant(str + 1) : false);
  }

  QUILL_NODISCARD static constexpr char const* _r_slant(char const* const str_begin, char const* str) noexcept
  {
    // clang-format off
    return str != str_begin ? (*str == path_delimiter ? (str + 1)
                                                      : _r_slant( str_begin, str -1))
                            : str;
    // clang-format on
  }

  QUILL_NODISCARD static constexpr char const* _extract_source_file_name(char const* str) noexcept
  {
    return _str_slant(str) ? _r_slant(str, _str_end(str)) : str;
  }

  QUILL_NODISCARD static constexpr char const* _log_level_to_string(LogLevel log_level)
  {
    constexpr std::array<char const*, 10> log_levels_strings = {
      {"TRACE_L3 ", "TRACE_L2 ", "TRACE_L1 ", "DEBUG    ", "INFO     ", "WARNING  ", "ERROR    ",
       "CRITICAL ", "BACKTRACE", "NONE"}};

    using log_lvl_t = std::underlying_type<LogLevel>::type;
    return log_levels_strings[static_cast<log_lvl_t>(log_level)];
  }

private:
  char const* _func{nullptr};
  char const* _pathname{nullptr};
  char const* _filename{nullptr};
  char const* _message_format{nullptr};
  char const* _lineno{nullptr};
  LogLevel _level{LogLevel::None};
};

} // namespace detail
} // namespace quill