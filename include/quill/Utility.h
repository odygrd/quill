/**
 * @page copyright
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
QUILL_NODISCARD std::string to_hex(T const* buffer, size_t size)
{
  static constexpr char hex_chars[] = "0123456789ABCDEF";

  if (!buffer || size == 0)
  {
    return {};
  }

  // Each byte needs 2 hex chars, and all but the last one need spaces
  std::string hex_string;
  hex_string.reserve(size > 0 ? (3 * size - 1) : 0);

  for (size_t i = 0; i < size; ++i)
  {
    const auto byte = static_cast<uint8_t>(buffer[i]);

    // Add the first four bits
    hex_string += hex_chars[(byte >> 4) & 0x0F];
    // Add the remaining bits
    hex_string += hex_chars[byte & 0x0F];

    // Add a space delimiter after all but the last byte
    if (i != (size - 1))
    {
      hex_string += ' ';
    }
  }

  return hex_string;
}
} // namespace utility

QUILL_END_NAMESPACE