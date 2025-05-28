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
 * @param uppercase If true, use uppercase hex characters (A-F); if false, use lowercase (a-f).
 * @return A string containing the hexadecimal representation of the given buffer.
 */
template <typename T>
QUILL_NODISCARD std::string to_hex(T const* buffer, size_t size, bool uppercase = true)
{
  static_assert(sizeof(T) == 1, "to_hex only accepts byte-sized element types");

  static constexpr char hex_chars_upper[] = "0123456789ABCDEF";
  static constexpr char hex_chars_lower[] = "0123456789abcdef";

  if (QUILL_UNLIKELY(!buffer || size == 0))
  {
    return std::string{};
  }

  // Select the appropriate character set based on the uppercase parameter
  char const* hex_chars = uppercase ? hex_chars_upper : hex_chars_lower;

  // Each byte needs 2 hex chars, and all but the last one need spaces
  std::string hex_string;
  hex_string.resize(3 * size - 1);

  size_t pos = 0;

  for (size_t i = 0; i < size; ++i)
  {
    auto const byte = static_cast<uint8_t>(buffer[i]);

    // Add the first four bits (high nibble)
    hex_string[pos++] = hex_chars[byte >> 4];
    // Add the remaining bits (low nibble)
    hex_string[pos++] = hex_chars[byte & 0x0F];

    // Add a space delimiter after all but the last byte
    if (i < size - 1)
    {
      hex_string[pos++] = ' ';
    }
  }

  return hex_string;
}
} // namespace utility

QUILL_END_NAMESPACE