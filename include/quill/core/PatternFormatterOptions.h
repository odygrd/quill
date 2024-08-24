/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include <cstdint>
#include <string>

QUILL_BEGIN_NAMESPACE

/**
 * @brief Configuration options for the PatternFormatter.
 *
 * This class encapsulates the configuration options used to customize
 * the formatting of log messages.
 */
class PatternFormatterOptions
{
public:
  /***/
  explicit PatternFormatterOptions(std::string format_pattern =
                                     "%(time) [%(thread_id)] %(short_source_location:<28) "
                                     "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   std::string timestamp_pattern = "%H:%M:%S.%Qns",
                                   Timezone timestamp_timezone = Timezone::LocalTime,
                                   bool add_metadata_to_multi_line_logs = true)
    : format_pattern(static_cast<std::string&&>(format_pattern)),
      timestamp_pattern(static_cast<std::string&&>(timestamp_pattern)),
      timestamp_timezone(timestamp_timezone),
      add_metadata_to_multi_line_logs(add_metadata_to_multi_line_logs)
  {
  }

  /**
   * @brief The format pattern for log messages.
   *
   * This string defines the overall structure of each log message.
   *
   * It can include various placeholders that will be replaced with actual values when formatting the log message.
   *
   * %(time)                    - Human-readable timestamp representing when the log statement was created.
   * %(file_name)               - Name of the source file where the logging call was issued.
   * %(full_path)               - Full path of the source file where the logging call was issued.
   * %(caller_function)         - Name of the function containing the logging call.
   * %(log_level)               - Textual representation of the logging level for the message.
   * %(log_level_short_code)    - Abbreviated log level name.
   * %(line_number)             - Line number in the source file where the logging call was issued.
   * %(logger)                  - Name of the logger used to log the call.
   * %(message)                 - The logged message itself.
   * %(thread_id)               - ID of the thread in which the logging call was made.
   * %(thread_name)             - Name of the thread. Must be set before the first log statement on that thread.
   * %(process_id)              - ID of the process in which the logging call was made.
   * %(source_location)         - Full source file path and line number as a single string.
   * %(short_source_location)   - Shortened source file name and line number as a single string.
   * %(tags)                    - Additional custom tags appended to the message when _TAGS macros are used.
   * %(named_args)              - Key-value pairs appended to the message. Only applicable when the message has named args; remains empty otherwise.
   *
   * @warning The same attribute cannot be used twice in the same format pattern.
   */
  std::string format_pattern;

  /**
   * @brief The format pattern for timestamps.
   *
   * This string defines how timestamps are formatted in log messages.
   * It follows the strftime() format with additional specifiers:
   * - %Qms : Milliseconds
   * - %Qus : Microseconds
   * - %Qns : Nanoseconds
   */
  std::string timestamp_pattern;

  /**
   * @brief The timezone to use for timestamps.
   *
   * Determines whether timestamps are formatted in local time or GMT.
   */
  Timezone timestamp_timezone;

  /**
   * @brief Whether to add metadata to each line of multi-line log messages.
   *
   * If true, ensures that metadata (e.g., timestamp, log level) is added
   * to every line of multi-line log entries, maintaining consistency
   * across all log outputs.
   */
  bool add_metadata_to_multi_line_logs;

  /***/
  bool operator==(const PatternFormatterOptions& other) const
  {
    return format_pattern == other.format_pattern && timestamp_pattern == other.timestamp_pattern &&
      timestamp_timezone == other.timestamp_timezone &&
      add_metadata_to_multi_line_logs == other.add_metadata_to_multi_line_logs;
  }

  /***/
  bool operator!=(const PatternFormatterOptions& other) const { return !(*this == other); }
};

QUILL_END_NAMESPACE