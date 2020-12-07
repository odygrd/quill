/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"

#if defined(__aarch64__)
#elif defined(__ARM_ARCH)
  #include <chrono>
#elif (defined(_M_ARM) || defined(_M_ARM64))
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
  // System timer of ARMv8 runs at a different frequency than the CPU's.
  // The frequency is fixed, typically in the range 1-50MHz.  It can be
  // read at CNTFRQ special register.  We assume the OS has set up the virtual timer properly.
  int64_t virtual_timer_value;
  __asm__ volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
  return virtual_timer_value;
}
#elif defined(__ARM_ARCH)
QUILL_NODISCARD_ALWAYS_INLINE_HOT uint64_t rdtsc() noexcept
{
  #if (__ARM_ARCH >= 6)
  // V6 is the earliest arch that has a standard cyclecount
  uint32_t pmccntr;
  uint32_t pmuseren;
  uint32_t pmcntenset;

  __asm__ volatile("mrc p15, 0, %0, c9, c14, 0" : "=r"(pmuseren));
  if (pmuseren & 1)
  {
    __asm__ volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcntenset));
    if (pmcntenset & 0x80000000ul)
    {
      __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr));
      return (static_cast<uint64_t>(pmccntr)) * 64u;
    }
  }
  #endif

  // soft failover
  return static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
}
#elif (defined(_M_ARM) || defined(_M_ARM64))
QUILL_NODISCARD_ALWAYS_INLINE_HOT uint64_t rdtsc() noexcept
{
  // soft failover
  return static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
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