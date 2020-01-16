#pragma once

#include <cstdint>

#include "quill/LogLevel.h"

namespace quill::detail
{

/**
 * Stores information about the logged line in compile time
 */
class LogLineInfo
{
public:
  constexpr LogLineInfo(uint32_t line, char const* filename, char const* function, char const* format, LogLevel log_level)
    : _function(function),
      _file_name(_extract_source_file_name(filename)),
      _format(format),
      _line(line),
      _log_level(log_level)
  {
  }

  /**
   * @return The function name
   */
  [[nodiscard]] constexpr char const* function_name() const noexcept { return _function; }

  /**
   * @return The file name
   */
  [[nodiscard]] constexpr char const* file_name() const noexcept { return _file_name; }

  /**
   * @return The user provided format
   */
  [[nodiscard]] constexpr char const* format() const noexcept { return _format; }

  /**
   * @return Log level as string
   */
  [[nodiscard]] constexpr char const* log_level_str() const noexcept
  {
    return _log_level_to_string(_log_level);
  }

  /**
   * @return The line number
   */
  [[nodiscard]] constexpr uint32_t line() const noexcept { return _line; }

  /**
   * @return The log level of the line
   */
  [[nodiscard]] constexpr LogLevel log_level() const noexcept { return _log_level; }

private:
  [[nodiscard]] static constexpr char const* _str_end(char const* str) noexcept
  {
    return *str ? _str_end(str + 1) : str;
  }

  [[nodiscard]] static constexpr bool _str_slant(char const* str) noexcept
  {
    return *str == '/' ? true : (*str ? _str_slant(str + 1) : false);
  }

  [[nodiscard]] static constexpr char const* _r_slant(char const* str) noexcept
  {
    return *str == '/' ? (str + 1) : _r_slant(str - 1);
  }

  [[nodiscard]] static constexpr char const* _extract_source_file_name(char const* str) noexcept
  {
    return _str_slant(str) ? _r_slant(_str_end(str)) : str;
  }

  [[nodiscard]] static constexpr char const* _log_level_to_string(LogLevel log_level)
  {
    using log_lvl_t = std::underlying_type<LogLevel>::type;
    return log_levels_strings[static_cast<log_lvl_t>(log_level)];
  }

private:
  static constexpr std::array<char const*, 9> log_levels_strings = {
    {"LOG_TRACE_L3", "LOG_TRACE_L2", "LOG_TRACE_L1", "LOG_DEBUG", "LOG_INFO", "LOG_WARNING",
     "LOG_ERROR", "LOG_CRITICAL", "LOG_NONE"}};

  char const* _function;
  char const* _file_name;
  char const* _format;
  uint32_t _line;
  LogLevel _log_level;
};

} // namespace quill::detail