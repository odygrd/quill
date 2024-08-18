/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "quill/core/Attributes.h"

QUILL_BEGIN_NAMESPACE

namespace utility
{
/**
 * @brief Formats the given buffer to hexadecimal representation.
 *
 * This function converts the contents of the input buffer to a hexadecimal string.
 *
 * @param buffer Pointer to the input buffer.
 * @param size Size of the input buffer.
 * @return A string containing the hexadecimal representation of the given buffer.
 */
template <typename T>
QUILL_NODISCARD std::string to_hex(T* buffer, size_t size) noexcept
{
  static constexpr char hex_chars[] = "0123456789ABCDEF";

  std::string hex_string;
  hex_string.reserve(3 * size);

  for (size_t i = 0; i < size; ++i)
  {
    // 00001111 mask
    static constexpr uint8_t mask = 0x0Fu;

    // add the first four bits
    hex_string += hex_chars[(buffer[i] >> 4u) & mask];

    // add the remaining bits
    hex_string += hex_chars[buffer[i] & mask];

    if (i != (size - 1))
    {
      // add a space delimiter
      hex_string += ' ';
    }
  }

  return hex_string;
}
} // namespace utility

QUILL_END_NAMESPACE