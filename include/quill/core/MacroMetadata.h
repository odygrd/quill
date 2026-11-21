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
    LoggerRemovalRequest,
    Metric,
    MdcSet,
    MdcErase,
    MdcClear,
    DynamicMetric
  };

  constexpr MacroMetadata() = default;

  constexpr MacroMetadata(char const* source_location, char const* caller_function,
                          char const* message_format, char const* tags, LogLevel log_level,
                          Event event, bool is_rate_limited = false) noexcept
    : _source_location(source_location),
      _caller_function(caller_function),
      _message_format(message_format),
      _tags(tags),
      _message_format_size(static_cast<uint32_t>(std::string_view{message_format}.size())),
      _colon_separator_pos(_calc_colon_separator_pos()),
      _file_name_pos(_calc_file_name_pos()),
      _log_level(log_level),
      _event(event),
      _is_rate_limited(is_rate_limited),
      _has_named_args(contains_named_args(std::string_view{message_format, _message_format_size}))
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
  QUILL_NODISCARD size_t message_format_size() const noexcept { return _message_format_size; }

  QUILL_NODISCARD char const* line() const noexcept
  {
    if (QUILL_UNLIKELY(_source_location[_colon_separator_pos] == '\0'))
    {
      return _source_location + _colon_separator_pos;
    }

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

  QUILL_NODISCARD constexpr bool has_named_args() const noexcept { return _has_named_args; }

  QUILL_NODISCARD Event event() const noexcept { return _event; }

  QUILL_NODISCARD bool is_rate_limited() const noexcept { return _is_rate_limited; }

  /**
   * @return true if a replacement field, including one nested in a format spec such as the
   * width in "{:{width}}", starts with a name rather than an index or its format spec.
   */
  QUILL_NODISCARD static constexpr bool contains_named_args(std::string_view fmt) noexcept
  {
    size_t pos{0};

    while (pos < fmt.length())
    {
      if (fmt[pos] != '{')
      {
        ++pos;
        continue;
      }

      ++pos; // consume {
      if (pos >= fmt.length())
      {
        break;
      }

      // first character after the {
      char const fc = fmt[pos];
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
        return true;
      }
    }

    return false;
  }

private:
  /***/
  QUILL_NODISCARD constexpr uint16_t _calc_file_name_pos() const noexcept
  {
    char const* source_location = _source_location;
    char const* file = source_location;
    while (*source_location)
    {
      char const cur = *source_location++;

      // Windows toolchains, and builds hosted on Windows, can spell source paths with either
      if ((cur == '/') || (cur == '\\'))
      {
        file = source_location;
      }
    }
    return static_cast<uint16_t>(file - _source_location);
  }

  /***/
  QUILL_NODISCARD constexpr uint16_t _calc_colon_separator_pos() const noexcept
  {
    // std::string_view::rfind is only constexpr from C++20
    size_t length = 0;
    while (_source_location[length] != '\0')
    {
      ++length;
    }

    for (size_t i = length; i > 0; --i)
    {
      if (_source_location[i - 1] == ':')
      {
        return static_cast<uint16_t>(i - 1);
      }
    }

    return static_cast<uint16_t>(length);
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
  bool _has_named_args{false};
};

QUILL_END_EXPORT

static_assert(sizeof(MacroMetadata) <= detail::QUILL_CACHE_LINE_SIZE,
              "Size of MacroMetadata exceeds the cache line size");

QUILL_END_NAMESPACE
