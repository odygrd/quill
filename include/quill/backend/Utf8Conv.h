/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include "quill/core/Attributes.h"

  #include <cstddef>
  #include <cstring>
  #include <limits>
  #include <memory>
  #include <string>
  #include <windows.h>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * @brief Convert a wide Unicode string to a UTF-8 encoded string.
 *
 * @param data Pointer to the wide string data.
 * @param wide_str_len Length of the wide string.
 * @return The UTF-8 encoded string.
 *
 * @remarks If the input string is empty or the conversion fails, an empty string is returned.
 */
QUILL_NODISCARD QUILL_EXPORT QUILL_ATTRIBUTE_USED inline std::string utf8_encode(std::byte const* data, size_t wide_str_len)
{
  // Check if the input is empty or exceeds the maximum length WideCharToMultiByte accepts.
  // Also reject lengths where allocating (wide_str_len + 1) wchar_t would overflow size_t,
  // which can happen on 32-bit targets where size_t is the same width as the length.
  constexpr size_t max_wide_len = ((std::numeric_limits<size_t>::max)() / sizeof(wchar_t)) - 1u;
  if (wide_str_len == 0 || wide_str_len > static_cast<size_t>((std::numeric_limits<int>::max)()) ||
      wide_str_len > max_wide_len)
  {
    return std::string{};
  }

  // Create a unique_ptr to hold the buffer and one for the null terminator
  std::unique_ptr<wchar_t[]> wide_buffer{new wchar_t[wide_str_len + 1]};

  // Because we are using a std::byte* buffer and the data are coming from there we will cast
  // back the data to char* and then copy them to a wide string buffer before accessing them
  std::memcpy(wide_buffer.get(), data, wide_str_len * sizeof(wchar_t));
  wide_buffer[wide_str_len] = L'\0';

  // Calculate the size needed for the UTF-8 string
  int const size_needed = WideCharToMultiByte(
    CP_UTF8, 0, wide_buffer.get(), static_cast<int>(wide_str_len), nullptr, 0, nullptr, nullptr);

  // Check for conversion failure
  if (size_needed == 0)
  {
    return std::string{};
  }

  // Create a buffer to hold the UTF-8 string
  std::string ret_val(static_cast<size_t>(size_needed), 0);

  // Convert the wide string to UTF-8
  if (WideCharToMultiByte(CP_UTF8, 0, wide_buffer.get(), static_cast<int>(wide_str_len),
                          &ret_val[0], size_needed, nullptr, nullptr) == 0)
  {
    // conversion failure
    return std::string{};
  }

  return ret_val;
}

QUILL_NODISCARD QUILL_EXPORT QUILL_ATTRIBUTE_USED inline std::string utf8_encode(std::wstring_view str)
{
  return utf8_encode(reinterpret_cast<std::byte const*>(str.data()), str.size());
}
} // namespace detail

QUILL_END_NAMESPACE

#endif