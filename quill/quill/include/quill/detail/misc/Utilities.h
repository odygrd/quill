/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

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
 * Constexpr string length
 * @param str
 * @return
 */
QUILL_NODISCARD constexpr size_t strlen(char const* str) { return *str ? 1 + strlen(str + 1) : 0; }

/**
 * Convert a string to wstring
 * @param str
 */
QUILL_NODISCARD std::wstring s2ws(std::string const& str) noexcept;

/**
 * wstring to string
 * @param wstr
 */
QUILL_NODISCARD std::string ws2s(std::wstring const& wstr) noexcept;

/**
 * Same as strncpy.
 * Compared to normal strncpy :
 * a) It copies only the required bytes and b) always null terminates.
 * @tparam N
 * @param destination
 * @param source
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
 * @tparam alignment
 * @tparam T
 * @param pointer
 * @return
 */
template <uint64_t alignment, typename T>
QUILL_NODISCARD_ALWAYS_INLINE_HOT constexpr T* align_pointer(void* pointer) noexcept
{
  static_assert(is_pow_of_two(alignment), "alignment must be a power of two");
  return reinterpret_cast<T*>((reinterpret_cast<uintptr_t>(pointer) + (alignment - 1ul)) & ~(alignment - 1ul));
}
} // namespace detail
} // namespace quill