/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"           // for memory_buffer
#include "quill/MacroMetadata.h" // for MacroMetadata
#include "quill/QuillError.h"    // for QUILL_THROW, Quil...
#include "quill/detail/LoggerDetails.h"
#include "quill/detail/backend/TimestampFormatter.h" // for TimestampFormatter
#include "quill/detail/misc/Attributes.h"            // for QUILL_NODISCARD
#include "quill/detail/misc/Common.h"                // for Timezone, Timezon...
#include "quill/detail/misc/Utilities.h"             // for strlength
#include <array>                                     // for array
#include <chrono>                                    // for nanoseconds
#include <cstddef>                                   // for size_t
#include <functional>                                // for function
#include <memory>                                    // for unique_ptr, make_...
#include <string>                                    // for string
#include <tuple>                                     // for make_tuple
#include <utility>                                   // for move, index_sequence
#include <vector>                                    // for vector

#include <iostream>

namespace quill
{
class PatternFormatter
{
  /** Public classes **/
public:
  /**
   * Stores the precision of the timestamp
   */
  enum class TimestampPrecision : uint8_t
  {
    None,
    MilliSeconds,
    MicroSeconds,
    NanoSeconds
  };

  enum Attribute : uint32_t
  {
    AsciiTime = 0,
    FileName,
    FunctionName,
    LevelName,
    LevelId,
    LineNo,
    LoggerName,
    PathName,
    Thread,
    ThreadName,
    Process,
    FileLine,
    Message,
    ATTR_NR_ITEMS
  };

  /** Main PatternFormatter class **/
public:
  /**
   * Constructor
   */
  PatternFormatter()
  {
    // Set the default pattern
    _set_pattern(
      "%(ascii_time) [%(thread)] %(fileline:<28) LOG_%(level_name) "
      "%(logger_name:<12) - %(message)");
  }

  /**
   * Constructor for a PatterFormatter with a custom format
   * @param format_pattern format_pattern a format string. Must be passed using the macro QUILL_STRING("format string");
   * @param timestamp_format The for format of the date. Same as strftime() format with extra specifiers `%Qms` `%Qus` `Qns`
   * @param timezone The timezone of the timestamp, local_time or gmt_time
   */
  PatternFormatter(std::string const& format_pattern, std::string const& timestamp_format, Timezone timezone)
    : _timestamp_formatter(timestamp_format, timezone)
  {
    _set_pattern(format_pattern);
  }

  PatternFormatter(PatternFormatter const& other) = delete;
  PatternFormatter(PatternFormatter&& other) noexcept = delete;
  PatternFormatter& operator=(PatternFormatter const& other) = delete;
  PatternFormatter& operator=(PatternFormatter&& other) noexcept = delete;

  /**
   * Destructor
   */
  ~PatternFormatter() = default;

  QUILL_ATTRIBUTE_HOT void format(std::chrono::nanoseconds timestamp, std::string_view thread_id,
                                  std::string_view thread_name, std::string_view process_id,
                                  std::string_view logger_name, MacroMetadata const& macro_metadata,
                                  fmt::memory_buffer const& log_msg);

  /**
   * Returns the stored formatted record, to be called after format(...) is called
   * @return Returns the stored formatted record
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT fmt::memory_buffer const& formatted_log_message() const noexcept
  {
    return _formatted_log_message;
  }

private:
  /**
   * Sets a pattern to the formatter.
   * The given pattern is broken down into three parts : part_1 - %(message) - part_2
   *
   * The following attribute names can be used with the corresponding placeholder in a %-style format string.
   * @note: The same attribute can not be used twice in the same format pattern
   *
   * %(ascii_time)    - Human-readable time when the LogRecord was created
   * %(filename)      - Source file where the logging call was issued
   * %(pathname)      - Full source file where the logging call was issued
   * %(function_name) - Name of function containing the logging call
   * %(level_name)    - Text logging level for the messageText logging level for the message
   * %(level_id)      - Single letter id
   * %(lineno)        - Source line number where the logging call was issued
   * %(logger_name)   - Name of the logger used to log the call.
   * %(message)       - The logged message
   * %(thread)        - Thread ID
   * %(thread_name)   - Thread Name if set
   * %(process)       - Process ID
   *
   * @throws on invalid format string
   */
  void _set_pattern(std::string const& format_pattern);

  /***/
  template <size_t I, typename T>
  void _set_arg(T const& arg)
  {
    _args[_order_index[I]] = fmt::detail::make_arg<fmt::format_context>(arg);
  }

  template <size_t I, typename T>
  QUILL_ALWAYS_INLINE_HOT void _set_arg_val(T const& arg)
  {
    fmt::detail::value<fmt::format_context>& value_ =
      *(reinterpret_cast<fmt::detail::value<fmt::format_context>*>(std::addressof(_args[_order_index[I]])));

    value_ = fmt::detail::arg_mapper<fmt::format_context>().map(arg);
  }

private:
  std::string _format;
  /** Each named argument in the format_pattern is mapped in order to this array **/
  std::array<size_t, Attribute::ATTR_NR_ITEMS> _order_index{};
  std::array<fmt::basic_format_arg<fmt::format_context>, Attribute::ATTR_NR_ITEMS> _args{};

  /** class responsible for formatting the timestamp */
  detail::TimestampFormatter _timestamp_formatter{"%H:%M:%S.%Qns", Timezone::LocalTime};

  /** The buffer where we store each formatted string, also stored as class member to avoid
   * re-allocations. This is mutable so we can have a format() const function **/
  fmt::memory_buffer _formatted_log_message;
};
} // namespace quill
