/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

/**
 * Convert number to string
 */
#define QUILL_AS_STR(x) #x
#define QUILL_STRINGIFY(x) QUILL_AS_STR(x)

namespace quill
{
namespace detail
{
/**
 * Cache line size
 */
static constexpr size_t CACHE_LINE_SIZE{64u};
static constexpr size_t CACHE_LINE_ALIGNED{2 * CACHE_LINE_SIZE};

constexpr bool detect_structured_log_template(std::string_view fmt)
{
  uint32_t pos{0};
  bool found_named_arg{false};

  // Iterates the format string and checks if any characters are contained inside `{}`
  while (pos < fmt.length())
  {
    if (fmt[pos] == '{')
    {
      ++pos; // consume {
      if (pos >= fmt.length())
      {
        break;
      }

      // first character after the {
      auto const fc = fmt[pos];
      if (fc == '{')
      {
        // this means first '{' was escaped, so we ignore both of them
        ++pos;
        continue;
      }

      // else look for the next '}'
      uint32_t char_cnt{0};
      while (pos < fmt.length())
      {
        if (fmt[pos] == '}')
        {
          ++pos; // consume }
          if (pos >= fmt.length())
          {
            break;
          }

          if (fmt[pos] == '}')
          {
            // this means first '}', was escaped ignore it
            ++pos;
            ++char_cnt;
            continue;
          }

          // we found '{' match, we can break
          break;
        }

        ++pos;
        ++char_cnt;
      }

      if ((char_cnt != 0) && ((fc >= 'a' && fc <= 'z') || (fc >= 'A' && fc <= 'Z')))
      {
        found_named_arg = true;
      }
    }
    ++pos;
  }

  return found_named_arg;
}

constexpr bool detect_structured_log_template(std::wstring_view)
{
  // structured log for wide chars is not supported. We always return false here, if the user provides a structured log with wide characters
  // we expected the fmt compile time format check to fail
  return false;
}

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
 * Round up to the next power of 2
 * @param n input
 * @return the next power of 2
 */
template <typename T>
QUILL_NODISCARD T next_power_of_2(T n)
{
  constexpr T max_power_of_2 = (std::numeric_limits<T>::max() >> 1) + 1;

  if (n >= max_power_of_2)
  {
    return max_power_of_2;
  }

  if (is_pow_of_two(static_cast<uint64_t>(n)))
  {
    return n;
  }

  T result = 1;
  while (result < n)
  {
    result <<= 1;
  }

  assert(is_pow_of_two(result) && "result is not a power of 2");

  return result;
}
} // namespace detail

/**
 * Enum to select a queue type
 */
enum class QueueType
{
  UnboundedBlocking,
  UnboundedDropping,
  UnboundedUnlimited,
  BoundedBlocking,
  BoundedDropping
};

/**
 * Enum to select a timezone
 */
enum class Timezone : uint8_t
{
  LocalTime,
  GmtTime
};

/**
 * Enum for the used clock type
 */
enum ClockSourceType : uint8_t
{
  Tsc,
  System,
  User
};

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(_WIN32)
  #pragma warning(push)
  #pragma warning(disable : 4265) // Disable warning about non-virtual destructor in MSVC
#endif

/**
 * Tags class that can be used for _WITH_TAGS log macros
 */
class Tags
{
public:
  constexpr Tags() = default;
  virtual void format(std::string&) const = 0;
};

#if defined(__clang__)
  #pragma clang diagnostic pop
#elif defined(__GNUC__)
  #pragma GCC diagnostic pop
#elif defined(_WIN32)
  #pragma warning(pop)
#endif

} // namespace quill
