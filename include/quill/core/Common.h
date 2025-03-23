/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"

#include <cstddef>
#include <cstdint>

/**
 * Convert number to string
 */
#define QUILL_AS_STR(x) #x
#define QUILL_STRINGIFY(x) QUILL_AS_STR(x)

#if !defined(QUILL_THREAD_LOCAL)
  #if defined(__GNUC__) && defined(__linux__)
    #define QUILL_THREAD_LOCAL __thread
  #else
    #define QUILL_THREAD_LOCAL thread_local
  #endif
#endif

#if !defined(QUILL_MAGIC_SEPARATOR)
  #define QUILL_MAGIC_SEPARATOR "\x01\x02\x03"
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
static constexpr size_t QUILL_CACHE_LINE_SIZE{64u};
static constexpr size_t QUILL_CACHE_LINE_ALIGNED{2 * QUILL_CACHE_LINE_SIZE};
} // namespace detail

/**
 * Enum to select a queue type
 */
enum class QueueType
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
enum class HugePagesPolicy
{
  Never,  // Do not use huge pages
  Always, // Use huge pages, fail if unavailable
  Try     // Try huge pages, but fall back to normal pages if unavailable
};

QUILL_END_NAMESPACE