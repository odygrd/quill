/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"

#if (__ARM_ARCH >= 6)
  #include <sys/time.h>
#else
  // assume x86-64 ..
  #if defined(_WIN32)
    #include <intrin.h>
  #else
    #include <x86intrin.h>
  #endif
#endif

namespace quill
{
namespace detail
{
#if defined(__aarch64__)
// arm64
QUILL_NODISCARD_ALWAYS_INLINE_HOT uint64_t rdtsc() noexcept
{
  int64_t virtual_timer_value;
  asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
  return virtual_timer_value;
}
#elif ((__ARM_ARCH >= 6) || defined(_M_ARM64))
// V6 is the earliest arch that has a standard cyclecount
QUILL_NODISCARD_ALWAYS_INLINE_HOT uint64_t rdtsc() noexcept
{
  uint32_t pmccntr;
  uint32_t pmuseren;
  uint32_t pmcntenset;

  asm volatile("mrc p15, 0, %0, c9, c14, 0" : "=r"(pmuseren));
  if (pmuseren & 1)
  {
    asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcntenset));
    if (pmcntenset & 0x80000000ul)
    {
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr));
      return (static_cast<uint64_t>(pmccntr)) * 64u;
    }
  }

  // soft failover
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}
#else
/**
 * Get the TSC counter
 * @return rdtsc timestamp
 */
QUILL_NODISCARD_ALWAYS_INLINE_HOT uint64_t rdtsc() noexcept { return __rdtsc(); }
#endif

} // namespace detail
} // namespace quill