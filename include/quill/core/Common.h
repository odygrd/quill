/**
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

#if defined(__GNUC__) && defined(__linux__)
  #define QUILL_THREAD_LOCAL __thread
#else
  #define QUILL_THREAD_LOCAL thread_local
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * Cache line size
 */
static constexpr size_t CACHE_LINE_SIZE{64u};
static constexpr size_t CACHE_LINE_ALIGNED{2 * CACHE_LINE_SIZE};
} // namespace detail

/**
 * Enum to select a queue type
 */
enum class QueueType
{
  UnboundedBlocking,
  UnboundedDropping,
  UnboundedUnlimited,
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

QUILL_END_NAMESPACE