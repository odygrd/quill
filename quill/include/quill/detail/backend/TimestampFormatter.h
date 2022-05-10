/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/backend/StringFromTime.h"
#include "quill/detail/misc/Attributes.h" // for QUILL_NODISCARD
#include "quill/detail/misc/Common.h"     // for Timezone, Timezone::LocalTime
#include <chrono>                         // for nanoseconds
#include <cstddef>                        // for size_t
#include <cstdint>                        // for uint32_t, uint8_t
#include <string>                         // for string

/** forward declarations **/
struct tm;

namespace quill
{
namespace detail
{

/**
 * Formats a timestamp given a format string as a pattern. The format pattern uses the
 * same format specifiers as strftime() but with the following additional specifiers :
 * 1) %Qms - Milliseconds
 * 2) %Qus - Microseconds
 * 3) %Qns - Nanoseconds
 * @note %Qms, %Qus, %Qns specifiers are mutually exclusive
 * e.g given : "%I:%M.%Qms%p" the output would be "03:21.343PM"
 */
class TimestampFormatter
{
private:
  enum AdditionalSpecifier : uint8_t
  {
    None,
    Qms,
    Qus,
    Qns
  };

public:
  /**
   * Constructor
   * @param timestamp_format_string  format string
   * @param timezone_type local time or gmttime, defaults to local time
   * @throws on invalid format string
   */
  explicit TimestampFormatter(std::string const& timestamp_format_string,
                              Timezone timezone_type = Timezone::LocalTime);

  /**
   * Formats a strings given a timestamp
   * @param time_since_epoch the timestamp from epoch
   * @return formatted string
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view format_timestamp(std::chrono::nanoseconds time_since_epoch);

private:
  /**
   * Append fractional seconds to the formatted strings
   * @param extracted_fractional_seconds the fractional seconds extracted e.g 123 or 654332 etc
   */
  void _append_fractional_seconds(uint32_t extracted_fractional_seconds);

private:
  /** As class member to avoid re-allocating **/
  std::string _formatted_date;

  /** The format string is broken down to two parts. Before and after our additional specifiers */
  std::string _format_part_1;
  std::string _format_part_2;

  /** Strftime cache for both parts of the string */
  StringFromTime _strftime_part_1;
  StringFromTime _strftime_part_2;

  /** Timezone, Local time by default **/
  Timezone _timezone_type;

  /** fractional seconds */
  AdditionalSpecifier _additional_format_specifier{AdditionalSpecifier::None};
};

} // namespace detail
} // namespace quill