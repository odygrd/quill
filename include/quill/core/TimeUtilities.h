/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/QuillError.h"

#ifndef QUILL_MODULE
  #include <cerrno>
  #include <cstdint>
  #include <cstdlib>
  #include <cstring>
  #include <ctime>
  #include <string>
  #include <string_view>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
// Prefix a literal so a successful empty expansion is distinguishable from buffer exhaustion.
inline std::string prefixed_strftime_format(std::string_view pattern, QUILL_MAYBE_UNUSED Timezone timezone)
{
  std::string format{"x"};
  format += pattern;

#if defined(_WIN32)
  if (timezone == Timezone::GmtTime)
  {
    // The Windows CRT obtains %z and %Z from the local timezone even for a GMT tm.
    for (size_t pos = format.find('%'); pos != std::string::npos; pos = format.find('%', pos))
    {
      size_t spec = pos + 1;
      if ((spec < format.size()) && (format[spec] == '#'))
      {
        ++spec;
      }

      if ((spec < format.size()) && ((format[spec] == 'E') || (format[spec] == 'O')))
      {
        ++spec;
      }

      if (spec == format.size())
      {
        break;
      }

      char const code = format[spec];
      if ((code == 'z') || (code == 'Z'))
      {
        std::string_view const replacement = (code == 'z') ? "+0000" : "UTC";
        format.replace(pos, spec - pos + 1, replacement);
        pos += replacement.size();
      }
      else
      {
        pos = spec + 1;
      }
    }
  }
#endif

  return format;
}

/**
 * Calls strftime(). Sets `invalid_format` for a directive that the Windows C runtime does not
 * support, for which it would end the process.
 */
inline size_t checked_strftime(char* buffer, size_t size, char const* format, tm const& time, bool& invalid_format) noexcept
{
  invalid_format = false;

#if defined(_WIN32)
  // The directives of the Universal C runtime, checked before strftime() is entered: its
  // invalid parameter handler ends the process by default, and the debug runtime displays an
  // assertion first. This also covers MinGW, which cannot replace the handler for the call
  constexpr char conversions[] = "aAbBcCdDeFgGhHIjmMnprRStTuwUWVxXyYzZ%";
  for (char const* spec = std::strchr(format, '%'); spec; spec = std::strchr(spec + 1, '%'))
  {
    ++spec;
    if (*spec == '#')
    {
      ++spec;
    }

    if ((*spec == 'E') || (*spec == 'O'))
    {
      ++spec;
    }

    if ((*spec == '\0') || !std::strchr(conversions, *spec))
    {
      invalid_format = true;
      return 0;
    }
  }
#endif

  return std::strftime(buffer, size, format, &time);
}

/**
 * Formats `time` with strftime(), growing the buffer as needed.
 * @throws QuillError if the C runtime rejects the pattern or the result exceeds 64 KiB
 */
QUILL_NODISCARD inline std::string format_strftime(std::string_view pattern, tm const& time, Timezone timezone)
{
  constexpr size_t max_buffer_size{64 * 1024};
  std::string const format = prefixed_strftime_format(pattern, timezone);
  std::string buffer(32, '\0');

  while (true)
  {
    bool invalid_format{false};
    size_t const length = checked_strftime(buffer.data(), buffer.size(), format.c_str(), time, invalid_format);

    if (length != 0)
    {
      // Drop the prefix added by prefixed_strftime_format(), reusing the buffer's allocation
      buffer.resize(length);
      buffer.erase(0, 1);
      return buffer;
    }

    if (QUILL_UNLIKELY(invalid_format || (buffer.size() >= max_buffer_size)))
    {
      QUILL_THROW(
        QuillError{"strftime failed to format the timestamp pattern \"" + std::string{pattern} +
                   "\". It may contain a format specifier the C runtime does not support."});
    }

    buffer.resize(buffer.size() * 2);
  }
}

/**
 * Portable gmtime_r or _s per operating system
 * @param timer to a time_t object to convert
 * @param buf to a struct tm object to store the result
 * @return copy of the buf pointer, or throws on error
 * @throws QuillError on failure
 */
inline tm* gmtime_rs(time_t const* timer, tm* buf)
{
#if defined(_WIN32)
  errno_t const res = gmtime_s(buf, timer);
  if (res)
  {
    QUILL_THROW(QuillError{
      std::string{"failed to call gmtime_rs, with error message errno: " + std::to_string(res)}});
  }
  return buf;
#else
  tm* res = gmtime_r(timer, buf);
  if (QUILL_UNLIKELY(!res))
  {
    QUILL_THROW(QuillError{std::string{"failed to call gmtime_rs, with error message errno: " +
                                       std::to_string(errno) + " error: " + std::strerror(errno)}});
  }
  return res;
#endif
}

/**
 * Portable localtime_r or _s per operating system
 * @param timer to a time_t object to convert
 * @param buf to a struct tm object to store the result
 * @return copy of the buf pointer, or throws on error
 * @throws QuillError on failure
 */
inline tm* localtime_rs(time_t const* timer, tm* buf)
{
#if defined(_WIN32)
  auto const res = localtime_s(buf, timer);
  if (res)
  {
    QUILL_THROW(QuillError{
      std::string{"failed to call localtime_rs, with error message errno: " + std::to_string(res)}});
  }
  return buf;
#else
  tm* res = localtime_r(timer, buf);
  if (QUILL_UNLIKELY(!res))
  {
    QUILL_THROW(QuillError{std::string{"failed to call localtime_rs, with error message errno: " +
                                       std::to_string(errno) + " error: " + std::strerror(errno)}});
  }
  return res;
#endif
}

/**
 * inverses of gmtime
 * @param tm struct tm to convert
 * @throws on invalid input
 */
inline time_t timegm(tm* tm)
{
#if defined(_WIN32)
  errno = 0;
  time_t const ret_val = ::_mkgmtime(tm);

  if (QUILL_UNLIKELY((ret_val == static_cast<time_t>(-1)) && (errno != 0)))
  {
    QUILL_THROW(QuillError{"_mkgmtime failed."});
  }

  return ret_val;
#else
  errno = 0;
  time_t const ret_val = ::timegm(tm);

  if (QUILL_UNLIKELY((ret_val == static_cast<time_t>(-1)) && (errno != 0)))
  {
    QUILL_THROW(QuillError{"timegm failed."});
  }

  return ret_val;
#endif
}
} // namespace detail

QUILL_END_NAMESPACE
