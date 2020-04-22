#pragma once

#include "quill/detail/misc/Attributes.h"

namespace quill
{
namespace detail
{

#if (__ARM_ARCH >= 6)
// V6 is the earliest arch that has a standard cyclecount
QUILL_NODISCARD_ALWAYS_INLINE_HOT uint64_t rdtsc() noexcept
{
  uint32_t pmccntr;
  uint32_t pmuseren;
  uint32_t pmcntenset;
  // Read the user mode perf monitor counter access permissions.
  asm volatile("mrc p15, 0, %0, c9, c14, 0" : "=r"(pmuseren));
  if (pmuseren & 1)
  {
    // Allows reading perfmon counters for user mode code.
    asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r"(pmcntenset));
    if (pmcntenset & 0x80000000ul)
    {
      // Is it counting?
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr));

      // The counter is set up to count every 64th cycle
      return (static_cast<uint64_t>(pmccntr)) * 64u;
    }
  }

  // soft failover
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

#else
  // assume x86-64 ..
  #if defined(_WIN32)
    #include <intrin.h>
  #else
    #include <x86intrin.h>
  #endif

/**
 * Get the TSC counter
 * @return rdtsc timestamp
 */
QUILL_NODISCARD_ALWAYS_INLINE_HOT uint64_t rdtsc() noexcept { return __rdtsc(); }

#endif

} // namespace detail
} // namespace quill