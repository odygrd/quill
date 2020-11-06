/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include <cstddef>
#include <sstream>
#include <string>

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
QUILL_NODISCARD std::string to_hex(unsigned char* buffer, size_t size) noexcept;
QUILL_NODISCARD std::string to_hex(unsigned char const* buffer, size_t size) noexcept;

/**
 * Formats the given buffer to hex
 * @param buffer input buffer
 * @param size input buffer size
 * @return A string containing the hexadecimal representation of the given buffer
 */
QUILL_NODISCARD std::string to_hex(char* buffer, size_t size) noexcept;
QUILL_NODISCARD std::string to_hex(char const* buffer, size_t size) noexcept;

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
  std::stringstream ss;
  ss << obj;
  return ss.str();
}
} // namespace utility
} // namespace quill