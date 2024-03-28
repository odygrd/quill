/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h" // for QUILL_NODISCARD, QUILL_NOD...
#include "quill/core/Common.h"     // for Timezone
#include "quill/core/Os.h"

#include <algorithm> // for min
#include <array>     // for array
#include <cassert>   // for assert
#include <chrono>
#include <cmath>
#include <codecvt>
#include <cstdint> // for uint64_t, uintptr_t
#include <cstdio>  // for size_t
#include <cstring> // for memcpy, strlen
#include <ctime>
#include <limits>
#include <locale>
#include <string> // for string, wstring
#include <vector>

namespace quill::detail
{
/**
 * Finds and replaces all occurrences of the old value in the given string
 * @param str The string we want to search and replace
 * @param old_value the old value to be replaced
 * @param new_value the new value
 */
inline void replace_all(std::string& str, std::string const& old_value, std::string const& new_value) noexcept
{
  std::string::size_type pos = 0u;
  while ((pos = str.find(old_value, pos)) != std::string::npos)
  {
    str.replace(pos, old_value.length(), new_value);
    pos += new_value.length();
  }
}

/**
 * Same as strncpy.
 * Compared to normal strncpy :
 * a) It copies only the required bytes and b) always null terminates.
 * @tparam N max buffer size
 * @param destination destination buffer
 * @param source source string
 */
template <size_t N>
void inline safe_strncpy(std::array<char, N>& destination, char const* source) noexcept
{
  assert(source && "source pointer can not be nullptr");
  size_t const len = (std::min)(std::strlen(source), N - 1);
  std::memcpy(destination.data(), source, len);
  destination[len] = '\0';
}

/**
 * Calculates the time from epoch of the nearest hour
 * @param timestamp timestamp
 * @return the time from epoch of the nearest hour
 */
QUILL_NODISCARD inline time_t nearest_hour_timestamp(time_t timestamp) noexcept
{
  time_t const nearest_hour_ts = timestamp - (timestamp % 3600);
  return nearest_hour_ts;
}

/**
 * Calculates the time from epoch till the next hour
 * @param timestamp timestamp
 * @return the time from epoch until the next hour
 */
QUILL_NODISCARD inline time_t next_hour_timestamp(time_t timestamp) noexcept
{
  time_t const next_hour_ts = nearest_hour_timestamp(timestamp) + 3600;
  return next_hour_ts;
}

/**
 * Calculates the time from epoch till next noon or midnight
 * @param timezone gmt or local time
 * @param timestamp timestamp
 * @return the time from epoch until next noon or midnight
 */
QUILL_NODISCARD inline time_t next_noon_or_midnight_timestamp(time_t timestamp, Timezone timezone) noexcept
{
  // Get the current date and time now as time_info
  tm time_info;

  if (timezone == Timezone::GmtTime)
  {
    gmtime_rs(&timestamp, &time_info);
  }
  else
  {
    localtime_rs(&timestamp, &time_info);
  }

  if (time_info.tm_hour < 12)
  {
    // we are before noon, so calculate noon
    time_info.tm_hour = 11;
    time_info.tm_min = 59;
    time_info.tm_sec = 59; // we add 1 second later
  }
  else
  {
    // we are after noon so we calculate midnight
    time_info.tm_hour = 23;
    time_info.tm_min = 59;
    time_info.tm_sec = 59; // we add 1 second later
  }

  // convert back to time since epoch
  std::chrono::system_clock::time_point const next_midnight = (timezone == Timezone::GmtTime)
    ? std::chrono::system_clock::from_time_t(detail::timegm(&time_info))
    : std::chrono::system_clock::from_time_t(std::mktime(&time_info));

  // returns seconds since epoch of the next midnight.
  return std::chrono::duration_cast<std::chrono::seconds>(next_midnight.time_since_epoch()).count() + 1;
}

/**
 * Calls strftime and returns a null terminated vector of chars
 * @param format_string The format string to pass to strftime
 * @param timestamp The timestamp
 * @param timezone local time or gmtime
 * @return the formatted string as vector of characters
 */
QUILL_NODISCARD inline std::vector<char> safe_strftime(char const* format_string, time_t timestamp, Timezone timezone)
{
  if (format_string[0] == '\0')
  {
    std::vector<char> res;
    res.push_back('\0');
    return res;
  }

  // Convert timestamp to time_info
  tm time_info;
  if (timezone == Timezone::LocalTime)
  {
    localtime_rs(reinterpret_cast<time_t const*>(std::addressof(timestamp)), std::addressof(time_info));
  }
  else if (timezone == Timezone::GmtTime)
  {
    gmtime_rs(reinterpret_cast<time_t const*>(std::addressof(timestamp)), std::addressof(time_info));
  }

  // Create a buffer to call strftimex
  std::vector<char> buffer;
  buffer.resize(32);
  size_t res = strftime(&buffer[0], buffer.size(), format_string, std::addressof(time_info));

  while (res == 0)
  {
    // if strftime fails we will reserve more space
    buffer.resize(buffer.size() * 2);
    res = strftime(&buffer[0], buffer.size(), format_string, std::addressof(time_info));
  }

  return buffer;
}
} // namespace quill::detail