/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>

#include "quill/common/Attributes.h"
#include "quill/common/Common.h"
#include "quill/common/Fmt.h"

/**
 * Contains useful utilities to assist with logging
 */
namespace quill
{
namespace utility
{
/**
 * Formats the given buffer to hex
 * @param buffer input buffer
 * @param size input buffer size
 * @return A string containing the hexadecimal representation of the given buffer
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
 * By default the logger will take a copy of the passed object and then will call the operator<< in the background thread
 * Use this function when :
 * 1) [to print a non copyable] It is not possible to take a copy of an object when the object is not copyable
 * 2) [to avoid race condition] You want to log a class that contains a reference or a pointer to another object as a class member, that
 * can be updated before the logger thread calls operator<<. In that case when the logger
 * thread tries to call operator<< on the class itself but the internal reference object might have changed between the
 * time you wanted to log and when the logged thread called operator <<.
 * Therefore, we need to accept the performance penalty calling operator<< on the caller thread
 * 3) Similar to 2) but the class contains a class that contains a shared_pointer or a raw pointer which gets modified
 * by the caller thread immediately after the logging call before the backend thread logs
 * @requires requires the custom type to have an operator<< overload defined
 * @param obj the given object
 * @return output of object's operator<< as std::string
 */
template <typename T>
QUILL_NODISCARD std::string to_string(T const& obj) noexcept
{
  return fmtquill::to_string(obj);
}

/**
 * Helper class for combining different CustomTag objects
 */
template <typename... TCustomTags>
class CombinedCustomTags : public CustomTags
{
public:
  constexpr CombinedCustomTags(TCustomTags... custom_tags, std::string_view delim = ", ")
    : _tags(std::move(custom_tags)...), _delim(delim)
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
  std::tuple<TCustomTags...> _tags;
  std::string_view _delim;
};
} // namespace utility
} // namespace quill