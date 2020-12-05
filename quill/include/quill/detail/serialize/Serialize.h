/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <cwchar>
#include <string>
#include <utility>

#include "quill/detail/misc/StringView.h"
#include "quill/detail/misc/Utilities.h"

namespace quill
{
namespace detail
{
/** Functions to get the total size of arguments **/

template <typename T>
constexpr size_t argument_size(T const&)
{
  return sizeof(T);
}

/**
 * For strings we append + 1 for the null terminator
 */
inline size_t argument_size(char* s) { return std::strlen(s) + 1; }

inline size_t argument_size(char const* s) { return std::strlen(s) + 1; }

inline size_t argument_size(std::string&& s) { return s.length() + 1; }

inline size_t argument_size(std::string const& s) { return s.length() + 1; }

template <typename T, size_t N>
constexpr size_t argument_size(T)
{
  // we just overload for all arrays, but we assume char array because type trait `is_serializable`
  // only allows char arrays anyway
  return N + 1;
}

#if defined(QUILL_USE_STRING_VIEW)
template <typename Char>
inline size_t argument_size(std_string_view<Char> const& s)
{
  return s.length() + 1;
}
#endif

/** Get the total argument size **/
inline void accumulate_arguments_size(size_t&)
{
  // no arguments
}

template <typename Arg>
inline void accumulate_arguments_size(size_t& total_size, Arg&& arg)
{
  total_size += argument_size(std::forward<Arg>(arg));
}

template <typename Arg, typename... Args>
inline void accumulate_arguments_size(size_t& total_size, Arg&& arg, Args&&... args)
{
  total_size += argument_size(std::forward<Arg>(arg));
  accumulate_arguments_size(total_size, std::forward<Args>(args)...);
}

/** End of Functions to get the total size of arguments **/

/** Serialize Argument **/

template <typename T>
inline void serialize_argument(unsigned char*& buffer, T arg)
{
  std::memcpy(buffer, &arg, argument_size(arg));
  buffer += sizeof(T);
}

/**
 * For strings we append + 1 for the null terminator
 */
inline void serialize_argument(unsigned char*& buffer, char* s)
{
  size_t const len = std::strlen(s);
  std::memcpy(buffer, s, len);
  buffer += len;
  *buffer = '\0';
  buffer += 1;
}

inline void serialize_argument(unsigned char*& buffer, char const* s)
{
  size_t const len = std::strlen(s);
  std::memcpy(buffer, s, len);
  buffer += len;
  *buffer = '\0';
  buffer += 1;
}

inline void serialize_argument(unsigned char*& buffer, std::string&& s)
{
  memcpy(buffer, s.data(), s.length());
  buffer += s.length();
  *buffer = '\0';
  buffer += 1;
}

inline void serialize_argument(unsigned char*& buffer, std::string const& s)
{
  memcpy(buffer, s.data(), s.length());
  buffer += s.length();
  *buffer = '\0';
  buffer += 1;
}

template <typename T, size_t N>
inline void serialize_argument(unsigned char*& buffer, T (&arr)[N])
{
  // we just overload for all arrays, but we assume char array because type trait `is_serializable`
  // only allows char arrays anyway
  memcpy(buffer, arr[0], N);
  buffer += N;
  *buffer = '\0';
  buffer += 1;
}

#if defined(QUILL_USE_STRING_VIEW)
template <typename Char>
inline void serialize_argument(unsigned char*& buffer, std_string_view<Char> const& s)
{
  memcpy(buffer, s.data(), s.length());
  buffer += s.length();
  *buffer = '\0';
  buffer += 1;
}
#endif

/** Serialize all arguments **/
inline void serialize_arguments(unsigned char*&)
{
  // no arguments
}

template <typename Arg>
inline void serialize_arguments(unsigned char*& buffer, Arg&& arg)
{
  serialize_argument(buffer, std::forward<Arg>(arg));
}

template <typename Arg, typename... Args>
inline void serialize_arguments(unsigned char*& buffer, Arg&& arg, Args&&... args)
{
  serialize_argument(buffer, std::forward<Arg>(arg));
  serialize_arguments(buffer, std::forward<Args>(args)...);
}

/** End of Serialize Argument **/
} // namespace detail
} // namespace quill