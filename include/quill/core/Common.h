/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"

#include <cstddef>
#include <cstdint>

#if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)

  #include <cstdio>
  #include <cstdlib>

#endif

#if !defined(QUILL_ASSERT)
  #if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)
    #define QUILL_ASSERT(expr, msg)                                                                \
      do                                                                                           \
      {                                                                                            \
        if QUILL_UNLIKELY (!(expr))                                                                \
        {                                                                                          \
          std::fprintf(stderr, "Quill assertion failed: %s, file %s, line %d. %s\n", #expr,        \
                       __FILE__, __LINE__, msg);                                                   \
          std::abort();                                                                            \
        }                                                                                          \
      } while (0)
  #else
    #define QUILL_ASSERT(expr, msg) ((void)0)
  #endif
#endif

#if !defined(QUILL_ASSERT_WITH_FMT)
  #if defined(QUILL_ENABLE_ASSERTIONS) || !defined(NDEBUG)
    #define QUILL_ASSERT_WITH_FMT(expr, fmt, ...)                                                  \
      do                                                                                           \
      {                                                                                            \
        if QUILL_UNLIKELY (!(expr))                                                                \
        {                                                                                          \
          char quill_assert_msg_buf[512];                                                          \
          std::snprintf(quill_assert_msg_buf, sizeof(quill_assert_msg_buf), fmt, __VA_ARGS__);     \
          std::fprintf(stderr, "Quill assertion failed: %s, file %s, line %d. %s\n", #expr,        \
                       __FILE__, __LINE__, quill_assert_msg_buf);                                  \
          std::abort();                                                                            \
        }                                                                                          \
      } while (0)
  #else
    #define QUILL_ASSERT_WITH_FMT(expr, fmt, ...) ((void)0)
  #endif
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * Cache line constants.
 *
 * Note: On FreeBSD, CACHE_LINE_SIZE is defined in a system header,
 * so we use a prefix to avoid naming conflicts.
 */
inline constexpr size_t QUILL_CACHE_LINE_SIZE{64u};
inline constexpr size_t QUILL_CACHE_LINE_ALIGNED{2 * QUILL_CACHE_LINE_SIZE};

// define our own PATH_PREFERRED_SEPARATOR to not include <filesystem>
#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
// MSVC on Windows: use backslashes
inline constexpr char PATH_PREFERRED_SEPARATOR = '\\';
#else
// Non-Windows OR Clang on Windows: use forward slashes
inline constexpr char PATH_PREFERRED_SEPARATOR = '/';
#endif
} // namespace detail

QUILL_BEGIN_EXPORT

/**
 * Enum to select a queue type
 */
enum class QueueType : uint8_t
{
  UnboundedBlocking,
  UnboundedDropping,
  BoundedBlocking,
  BoundedDropping
};

/**
 * Enum to select a timezone
 */
enum class Timezone : uint8_t
{
  LocalTime,
  GmtTime
};

/**
 * Enum for the used clock type
 */
enum class ClockSourceType : uint8_t
{
  Tsc,
  System,
  User
};

/**
 * Enum for huge pages
 */
enum class HugePagesPolicy : uint8_t
{
  Never,  // Do not use huge pages
  Always, // Use 2 MiB huge pages, fail if unavailable
  Try     // Try 2 MiB huge pages, but fall back to normal pages if unavailable
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
