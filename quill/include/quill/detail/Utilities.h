#pragma once

#include <cstdint>

namespace quill::detail
{

static constexpr uint32_t CACHELINE_SIZE{64};

/**
 * Check if a number is a power of 2
 * @param number the number to check against
 * @return true if the number is power of 2
 *         false otherwise
 */
[[nodiscard]] constexpr bool is_pow_of_two(uint64_t number) noexcept
{
  return (number != 0) && ((number & (number - 1)) == 0);
}

/**
 * Returns the os assigned id of the thread
 * @return
 */
[[nodiscard]] uint32_t get_thread_id() noexcept;

} // namespace quill::detail