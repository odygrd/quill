/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_NODISCARD, QUILL_NOD...
#include "quill/detail/misc/Common.h"     // for Timezone
#include <algorithm>                      // for min
#include <array>                          // for array
#include <cassert>                        // for assert
#include <cstdint>                        // for uint64_t, uintptr_t
#include <cstdio>                         // for size_t
#include <cstring>                        // for memcpy, strlen
#include <string>                         // for string, wstring
#include <vector>

namespace quill
{
namespace detail
{
/**
 * Check if a number is a power of 2
 * @param number the number to check against
 * @return true if the number is power of 2
 *         false otherwise
 */
QUILL_NODISCARD constexpr bool is_pow_of_two(uint64_t number) noexcept
{
  return (number != 0) && ((number & (number - 1)) == 0);
}

/**
 * Finds and replaces all occurrences of the old value in the given string
 * @param str The string we want to search and replace
 * @param old_value the old value to be replaced
 * @param new_value the new value
 */
void replace_all(std::string& str, std::string const& old_value, std::string const& new_value) noexcept;

/**
 * Convert a string to wstring
 * @param str input string
 * @return the value of input string as wide string
 */
QUILL_NODISCARD std::wstring s2ws(std::string const& str) noexcept;

/**
 * wstring to string
 * @param wstr input wide string
 * @return the value of input wide string as string
 */
QUILL_NODISCARD std::string ws2s(std::wstring const& wstr) noexcept;

/**
 * Same as strncpy.
 * Compared to normal strncpy :
 * a) It copies only the required bytes and b) always null terminates.
 * @tparam N max buffer size
 * @param destination destination buffer
 * @param source source string
 */
template <size_t N>
void safe_strncpy(std::array<char, N>& destination, char const* source) noexcept
{
  assert(source && "source pointer can not be nullptr");
  size_t const len = (std::min)(std::strlen(source), N - 1);
  std::memcpy(destination.data(), source, len);
  destination[len] = '\0';
}

/**
 * align a pointer to the given alignment
 * @tparam alignment desired alignment
 * @tparam T desired return type
 * @param pointer a pointer the object
 * @return an aligned pointer for the given object
 */
template <uint64_t alignment, typename T>
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT constexpr T* align_pointer(void* pointer) noexcept
{
  static_assert(is_pow_of_two(alignment), "alignment must be a power of two");
  return reinterpret_cast<T*>((reinterpret_cast<uintptr_t>(pointer) + (alignment - 1ul)) & ~(alignment - 1ul));
}

/**
 * Calculates the time from epoch of the nearest hour
 * @param timestamp timestamp
 * @return the time from epoch of the nearest hour
 */
QUILL_NODISCARD time_t nearest_hour_timestamp(time_t timestamp) noexcept;

/**
 * Calculates the time from epoch till the next hour
 * @param timestamp timestamp
 * @return the time from epoch until the next hour
 */
QUILL_NODISCARD time_t next_hour_timestamp(time_t timestamp) noexcept;

/**
 * Calculates the time from epoch till next noon or midnight
 * @param timezone gmt or local time
 * @param timestamp timestamp
 * @return the time from epoch until next noon or midnight
 */
QUILL_NODISCARD time_t next_noon_or_midnight_timestamp(time_t timestamp, Timezone timezone) noexcept;

/**
 * Calls strftime and returns a null terminated vector of chars
 * @param format_string The format string to pass to strftime
 * @param timestamp The timestamp
 * @param timezone local time or gmtime
 * @return the formatted string as vector of characters
 */
QUILL_NODISCARD std::vector<char> safe_strftime(char const* format_string, time_t timestamp, Timezone timezone);

/**
 * Split a string into tokens
 * @param s given string
 * @param delimiter delimiter
 * @return returns a vector of tokens
 */
QUILL_NODISCARD std::vector<std::string> split(std::string const& s, char delimiter);
} // namespace detail
} // namespace quill