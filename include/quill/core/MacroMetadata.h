/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/LogLevel.h"

#ifndef QUILL_MODULE
  #include <cstddef>
  #include <cstdint>
  #include <string_view>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * @return a view of `str`, measured with the builtin where it is a constant. The logging macros
 * construct the metadata of each statement in a constant evaluation, where libstdc++ measures a
 * C string with a loop that calls a function for each character. GCC cannot measure a string
 * generated into a constexpr array with the builtin, so that one still takes the loop
 */
QUILL_NODISCARD constexpr std::string_view c_string_view(char const* str) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
  if (__builtin_constant_p(__builtin_strlen(str)))
  {
    return std::string_view{str, __builtin_strlen(str)};
  }
#endif
  return std::string_view{str};
}
} // namespace detail

QUILL_BEGIN_EXPORT

/**
 * Captures and stores information about a logging event in compile time
 */
class MacroMetadata
{
public:
  enum Event : uint8_t
  {
    None,
    Log,
    InitBacktrace,
    FlushBacktrace,
    Flush,
    LogWithRuntimeMetadataDeepCopy,
    LogWithRuntimeMetadataHybridCopy,
    LogWithRuntimeMetadataShallowCopy,
    LogWithStaticRuntimeMetadata,
    LoggerRemovalRequest,
    Metric,
    MdcSet,
    MdcErase,
    MdcClear,
    DynamicMetric,
    Binary
  };

  constexpr MacroMetadata() = default;

  constexpr MacroMetadata(char const* source_location, char const* caller_function,
                          char const* message_format, char const* tags, LogLevel log_level,
                          Event event, bool is_rate_limited = false) noexcept
    : MacroMetadata(detail::c_string_view(source_location), caller_function,
                    detail::c_string_view(message_format), tags, log_level, event, is_rate_limited)
  {
  }

  QUILL_NODISCARD char const* source_location() const noexcept { return _source_location; }

  QUILL_NODISCARD char const* caller_function() const noexcept { return _caller_function; }

  QUILL_NODISCARD char const* message_format() const noexcept { return _message_format; }

  /**
   * The length of message_format(). It is calculated with the metadata, at compile time for the
   * logging macros, so that the backend does not measure the format string of every record.
   *
   * It is one of the facts that the metadata stores about its strings, like has_named_args() and
   * the positions of the file name and line. The strings must therefore not change while the
   * metadata is used: code that builds a MacroMetadata itself, e.g. for Logger::log_statement(),
   * constructs a new one for another format string instead of writing it to the same buffer.
   */
  QUILL_NODISCARD constexpr size_t message_format_size() const noexcept
  {
    return _message_format_size;
  }

  QUILL_NODISCARD char const* line() const noexcept
  {
    if (QUILL_UNLIKELY(_source_location[_colon_separator_pos] == '\0'))
    {
      return _source_location + _colon_separator_pos;
    }

    return _source_location + _colon_separator_pos + 1;
  }

  QUILL_NODISCARD constexpr std::string_view full_path() const noexcept
  {
    return std::string_view{_source_location, _colon_separator_pos};
  }

  QUILL_NODISCARD constexpr std::string_view file_name() const noexcept
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

  QUILL_NODISCARD constexpr bool has_named_args() const noexcept
  {
    return (_format_flags & _named_args_flag) != 0;
  }

  /**
   * @return true if each brace of message_format() is in a "{}" field, e.g. "price: {} qty: {}" or
   * a message without fields. Its arguments are then formatted in order without format specs
   */
  QUILL_NODISCARD constexpr bool has_only_empty_fields() const noexcept
  {
    return (_format_flags & _only_empty_fields_flag) != 0;
  }

  QUILL_NODISCARD Event event() const noexcept { return _event; }

  QUILL_NODISCARD bool is_rate_limited() const noexcept { return _is_rate_limited; }

  /**
   * @return true if a replacement field, including one nested in a format spec such as the
   * width in "{:{width}}", starts with a name rather than an index or its format spec.
   */
  QUILL_NODISCARD static constexpr bool contains_named_args(std::string_view fmt) noexcept
  {
    return (_scan_format(fmt) & _named_args_flag) != 0;
  }

  /** The flags of message_format(), for the constructor below */
  QUILL_NODISCARD constexpr uint8_t format_flags() const noexcept { return _format_flags; }

  /**
   * Constructs the metadata from the lengths, positions and format_flags() that were found at
   * compile time for the strings of a macro-free call, so the backend does not find them again.
   */
  constexpr MacroMetadata(char const* source_location, uint16_t colon_separator_pos,
                          uint16_t file_name_pos, char const* caller_function, char const* message_format,
                          uint32_t message_format_size, uint8_t format_flags, char const* tags,
                          LogLevel log_level, Event event, bool is_rate_limited = false) noexcept
    : _source_location(source_location),
      _caller_function(caller_function),
      _message_format(message_format),
      _tags(tags),
      _message_format_size(message_format_size),
      _colon_separator_pos(colon_separator_pos),
      _file_name_pos(file_name_pos),
      _log_level(log_level),
      _event(event),
      _is_rate_limited(is_rate_limited),
      _format_flags(format_flags)
  {
  }

  /**
   * Constructs the metadata from strings whose lengths are known, e.g. std::string, which must end
   * with a null character as source_location() and message_format() return them as C strings.
   *
   * The strings are measured once, here or in the views of the other constructor. The macro-free
   * functions construct the metadata of every record at run time, where measuring a string is a
   * call to strlen() and the positions below are found from the end of the source location: the
   * file name and the line follow a path that is usually much longer than both.
   */
  constexpr MacroMetadata(std::string_view source_location, char const* caller_function,
                          std::string_view message_format, char const* tags, LogLevel log_level,
                          Event event, bool is_rate_limited = false) noexcept
    : _source_location(source_location.data()),
      _caller_function(caller_function),
      _message_format(message_format.data()),
      _tags(tags),
      _message_format_size(static_cast<uint32_t>(message_format.size())),
      _colon_separator_pos(_calc_colon_separator_pos(source_location)),
      _file_name_pos(_calc_file_name_pos(source_location)),
      _log_level(log_level),
      _event(event),
      _is_rate_limited(is_rate_limited),
      _format_flags(_scan_format(message_format))
  {
  }

private:
  /** The flags that _scan_format() finds in a format string */
  static constexpr uint8_t _named_args_flag{1u};
  static constexpr uint8_t _only_empty_fields_flag{2u};

  /**
   * @return _named_args_flag if `fmt` contains named args, else _only_empty_fields_flag if each of
   * its braces is in a "{}" field, else 0. Both are found in one pass: the macro-free functions
   * construct the metadata of every record at run time
   */
  QUILL_NODISCARD static constexpr uint8_t _scan_format(std::string_view fmt) noexcept
  {
    // Index the characters through a pointer: a constant evaluation calls each std::string_view
    // member as a function, for every character of every log statement
    char const* const data = fmt.data();
    size_t const size = fmt.size();
    bool has_only_empty_fields{true};
    size_t pos{0};

    while (pos < size)
    {
      char const c = data[pos];
      ++pos;

      if (c == '}')
      {
        // A '}' that does not close a "{}" field, escaped or not
        has_only_empty_fields = false;
        continue;
      }

      if (c != '{')
      {
        continue;
      }

      if (pos >= size)
      {
        return 0;
      }

      // first character after the {
      char const fc = data[pos];
      if (fc == '}')
      {
        ++pos;
        continue;
      }

      has_only_empty_fields = false;

      if (fc == '{')
      {
        // this means first '{' was escaped, so we ignore both of them
        ++pos;
        continue;
      }

      // Scan on from the first character instead of skipping to the closing '}', so a nested field
      // such as the dynamic width in "{:{}} {name}" cannot hide a named field that follows it
      if ((fc >= 'a' && fc <= 'z') || (fc >= 'A' && fc <= 'Z') || (fc == '_'))
      {
        return _named_args_flag;
      }
    }

    return has_only_empty_fields ? _only_empty_fields_flag : uint8_t{0};
  }

  /** @return the position after the last path separator of `str`, 0 without one */
  QUILL_NODISCARD static constexpr uint16_t _calc_file_name_pos(std::string_view str) noexcept
  {
    char const* const data = str.data();

    for (size_t i = str.size(); i > 0; --i)
    {
      char const cur = data[i - 1];

      // Windows toolchains, and builds hosted on Windows, can spell source paths with either
      if ((cur == '/') || (cur == '\\'))
      {
        return static_cast<uint16_t>(i);
      }
    }

    return 0;
  }

  /**
   * @return the position of the last ':' after the last path separator of `str`, its length
   * without one. A ':' before the file name is not the one of the line, e.g. the one of a Windows
   * drive in a source location that has no line
   */
  QUILL_NODISCARD static constexpr uint16_t _calc_colon_separator_pos(std::string_view str) noexcept
  {
    // std::string_view::rfind is only constexpr from C++20
    char const* const data = str.data();

    for (size_t i = str.size(); i > 0; --i)
    {
      char const cur = data[i - 1];

      if (cur == ':')
      {
        return static_cast<uint16_t>(i - 1);
      }

      if ((cur == '/') || (cur == '\\'))
      {
        break;
      }
    }

    return static_cast<uint16_t>(str.size());
  }

private:
  char const* _source_location{nullptr};
  char const* _caller_function{nullptr};
  char const* _message_format{nullptr};
  char const* _tags{nullptr};
  uint32_t _message_format_size{0};
  uint16_t _colon_separator_pos{0};
  uint16_t _file_name_pos{0};
  LogLevel _log_level{LogLevel::None};
  Event _event{Event::None};
  bool _is_rate_limited{false};
  uint8_t _format_flags{0};
};

QUILL_END_EXPORT

static_assert(sizeof(MacroMetadata) <= detail::QUILL_CACHE_LINE_SIZE,
              "Size of MacroMetadata exceeds the cache line size");

namespace detail
{
/**
 * The record of a macro-free call whose format string is a literal, see LogFunctions.h: the
 * pointers to the strings of the call and the facts about them that the backend needs.
 *
 * packed holds, from the lowest bit:
 *   line          20 bits
 *   fmt_len       16 bits
 *   file_len      14 bits
 *   file_name_pos 12 bits
 *   format_flags   2 bits, MacroMetadata::format_flags()
 */
struct StaticRuntimeMetadata
{
  static constexpr uint32_t max_line{(1u << 20u) - 1u};
  static constexpr size_t max_fmt_len{(1u << 16u) - 1u};
  static constexpr size_t max_file_len{(1u << 14u) - 1u};
  static constexpr size_t max_file_name_pos{(1u << 12u) - 1u};

  /**
   * @return the facts packed, or 0 if one is above its maximum, e.g. the line of generated code.
   * A call that does not fit copies its format string and source location instead. A packed value
   * is never 0: a format string has a length or, when it is empty, only empty fields
   */
  QUILL_NODISCARD static constexpr uint64_t pack(uint32_t line, size_t fmt_len, size_t file_len,
                                                 size_t file_name_pos, uint8_t format_flags) noexcept
  {
    if ((line > max_line) || (fmt_len > max_fmt_len) || (file_len > max_file_len) ||
        (file_name_pos > max_file_name_pos))
    {
      return 0;
    }

    return static_cast<uint64_t>(line) | (static_cast<uint64_t>(fmt_len) << 20u) |
      (static_cast<uint64_t>(file_len) << 36u) | (static_cast<uint64_t>(file_name_pos) << 50u) |
      (static_cast<uint64_t>(format_flags) << 62u);
  }

  QUILL_NODISCARD constexpr uint32_t line() const noexcept
  {
    return static_cast<uint32_t>(packed & max_line);
  }

  QUILL_NODISCARD constexpr uint32_t fmt_len() const noexcept
  {
    return static_cast<uint32_t>((packed >> 20u) & max_fmt_len);
  }

  QUILL_NODISCARD constexpr uint16_t file_len() const noexcept
  {
    return static_cast<uint16_t>((packed >> 36u) & max_file_len);
  }

  QUILL_NODISCARD constexpr uint16_t file_name_pos() const noexcept
  {
    return static_cast<uint16_t>((packed >> 50u) & max_file_name_pos);
  }

  QUILL_NODISCARD constexpr uint8_t format_flags() const noexcept
  {
    return static_cast<uint8_t>(packed >> 62u);
  }

  char const* fmt;
  char const* file;
  char const* function;
  uint64_t packed;
};
} // namespace detail

QUILL_END_NAMESPACE
