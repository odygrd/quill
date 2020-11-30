/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"
#include <string>
#include <sstream>

/**
 * Common type definitions etc
 */
#if !defined(QUILL_RDTSC_RESYNC_INTERVAL)
  #define QUILL_RDTSC_RESYNC_INTERVAL 700
#endif

#if !defined(QUILL_ACTIVE_LOG_LEVEL)
  #define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3
#endif

#if !defined(QUILL_QUEUE_CAPACITY)
  #define QUILL_QUEUE_CAPACITY 131'072
#endif

namespace quill
{
namespace detail
{
/**
 * Cache line size
 */
static constexpr size_t CACHELINE_SIZE{64u};
} // namespace detail

/**
 * filename_t for windows/linux
 */
#if defined(_WIN32)
using filename_t = std::wstring;
using filename_ss_t = std::wstringstream;
  #define QUILL_FILENAME_STR(s) L##s
#else
using filename_t = std::string;
using filename_ss_t = std::stringstream;
  #define QUILL_FILENAME_STR(s) s
#endif

// Path delimiter
/**
 * Path delimiter windows/linux
 */
#if defined(_WIN32)
static constexpr char path_delimiter = '\\';
#else
static constexpr char path_delimiter = '/';
#endif

/**
 * Enum to select a timezone
 */
enum class Timezone : uint8_t
{
  LocalTime,
  GmtTime
};

} // namespace quill
