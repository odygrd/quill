/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"

/**
 * Contains useful utilities to assist with logging
 */
namespace quill::utility
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

/**
 * @brief Helper class for combining quill::Tag objects.
 * This class combines multiple quill::Tag objects into a single Tag object.
 */
template <typename... TTags>
class CombinedTags : public Tags
{
public:
  constexpr CombinedTags(TTags... tags, std::string_view delim = ", ")
    : _tags(std::move(tags)...), _delim(delim)
  {
  }

  void format(std::string& out) const override
  {
    std::apply([&out, this](const auto&... tags)
               { (((tags.format(out)), out.append(_delim.data())), ...); }, _tags);

    if (!out.empty())
    {
      // erase last delim
      out.erase(out.length() - _delim.length(), _delim.length());
    }
  }

private:
  std::tuple<TTags...> _tags;
  std::string_view _delim;
};
} // namespace quill::utility