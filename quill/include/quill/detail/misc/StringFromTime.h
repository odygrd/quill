/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace quill
{
namespace detail
{
/**
 * A class that converts a timestamp to a string based on the given format.
 * It works exactly the same as strftime() but it caches the string on it's creation
 * and only modifies the parts of the string that need to change.
 * This is much more efficient than calling strftime as we just need to calculate the different
 * between the cached and the requested timestamp and modify our preformatted string.
 *
 * 1) We take a format string eg "%Y-%m-%dT%H:%M:%SZ".
 * 2) We split it into several parts (_initial_parts) where we separate the time modifiers.
 * e.g "%Y-%m-%dT", "%H", ":", "%M", ":", "%S", Z".
 * 3) We cache the current timestamp and current h/m/s in seconds and on each part above
 * we call strftime() and we create a preformatted string.
 * 4) While calling stftime in 3) we can manually check for any time modifiers and store
 * their index in the final pre-formatted string.
 * 5) Next time we want to calculate the timestamp we will just calculate the difference
 * in seconds between the current and the cache timestamp.
 * Then we add this value to the cached seconds and then convert the seconds
 * to hh::mm::ss and replace in our pre-formatted string using the stored indexes.
 * 6) We re-calculate the pre-formatted string every midnight and noon.
 */
class StringFromTime
{
public:
  /**
   * Constructor
   */
  StringFromTime() = default;
  ~StringFromTime() = default;

  /**
   * Init and generate a pre-formatted string
   * @param timestamp_format the format of the timestamp
   * @param timezone the timezone, gmtime or localtime
   * @throws %X as format modifier not supported
   */
  QUILL_ATTRIBUTE_COLD void init(std::string timestamp_format, Timezone timezone);

  /**
   * Format a given timestamp using a cached string
   * @param timestamp timestamp to be formatted
   * @return the foramtted timestamp
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string const& format_timestamp(time_t timestamp);

private:
  enum class format_type : uint8_t
  {
    H,
    M,
    S,
    I,
    k,
    l,
    s
  };

  /**
   * Given a timestamp format string will split it into parts and store them in _initial_parts
   * @param timestamp_format the timestamp format string
   */
  QUILL_ATTRIBUTE_COLD void _populate_initial_parts(std::string timestamp_format);

  /**
   * Calls strftime on each part of _initial_parts and populates a pre_formatted string
   * and also caches the indexes of the time modifiers
   * @param timestamp the timestamp we want to use for the pre-formatted string
   */
  void _populate_pre_formatted_string_and_cached_indexes(time_t timestamp);

private:
  /** Contains the timestamp_format broken down into parts. We call use those parts to
   * create a pre-formatted string */
  std::vector<std::string> _initial_parts;

  /** Contains stored indexes of the _pre_formatted_ts for each format time modifier*/
  std::vector<std::pair<size_t, format_type>> _cached_indexes;

  /** The format request format of the timestamp. This can be slightly modified in constructor so we store it. */
  std::string _timestamp_format;

  /** The pre-formatted timestamp string */
  std::string _pre_formatted_ts;

  /** This is only used only when we fallback to strftime */
  std::string _fallback_formatted;

  /** The timestamp of the next noon, or midnight, we use this to resync */
  time_t _next_recalculation_timestamp{0};

  /** The timestamp of value of our pre-formated string */
  time_t _cached_timestamp{0};

  /** The seconds of hh:mm:ss of the pre-formatted string this is after using gmtime or localtime on cached_timestamp */
  uint32_t _cached_seconds{0};

  /** gmtime or localtime */
  Timezone _time_zone;
};
} // namespace detail
} // namespace quill