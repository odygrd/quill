/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"
#include <string>

/**
 * Common type definitions etc
 */

#if !defined(QUILL_RDTSC_RESYNC_INTERVAL)
  #define QUILL_RDTSC_RESYNC_INTERVAL 1
#endif

#if !defined(QUILL_ACTIVE_LOG_LEVEL)
  #define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3
#endif

#if !defined(QUILL_BOUNDED_SPSC_QUEUE_SIZE)
  #define QUILL_BOUNDED_SPSC_QUEUE_SIZE 2097152u
#endif

namespace quill
{
// Expose filename_t outside detail namespace
#if defined(_WIN32)
using filename_t = std::wstring;
#else
using filename_t = std::string;
#endif

} // namespace quill
