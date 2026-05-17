/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"

#include <cstddef>
#include <cstdint>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * Check if a number is a power of 2
 * @param number the number to check against
 * @return true if the number is power of 2
 *         false otherwise
 */
QUILL_NODISCARD constexpr bool is_power_of_two(uint64_t number) noexcept
{
  return (number != 0) && ((number & (number - 1)) == 0);
}

/**
 * Round up to the next power of 2
 * @param n input
 * @return the next power of 2
 */
QUILL_NODISCARD inline size_t next_power_of_two(size_t n) noexcept
{
  // Highest power of 2 representable in size_t: the top bit set.
  // SIZE_MAX comes from <cstdint> and avoids pulling <limits> on the frontend path.
  constexpr size_t max_power_of_2 = (SIZE_MAX >> 1) + 1u;

  if (n >= max_power_of_2)
  {
    return max_power_of_2;
  }

  if (is_power_of_two(static_cast<uint64_t>(n)))
  {
    return n;
  }

  size_t result = 1;
  while (result < n)
  {
    result <<= 1;
  }

  QUILL_ASSERT(is_power_of_two(static_cast<uint64_t>(result)),
               "result is not a power of 2 in next_power_of_two()");

  return result;
}
} // namespace detail

QUILL_END_NAMESPACE
