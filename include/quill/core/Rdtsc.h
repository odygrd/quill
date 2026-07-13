/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include <cstdint>

#if defined(__ARM_ARCH) || defined(_M_ARM) || defined(_M_ARM64) || defined(__PPC64__)
  // ARM or PowerPC — use ChronoTimeUtils for timestamping
  #include "quill/core/ChronoTimeUtils.h"

#elif defined(__riscv) || defined(__s390x__) || defined(__loongarch64)
// RISC-V, IBM Z (s390x), or LoongArch — no special intrinsics required

#else
  // Assume x86 or x86-64 platform
  #if __has_include(<intrin.h>) && defined(_WIN32)
    // Use Windows-specific intrinsics
    #include <intrin.h>
  #elif __has_include(<x86gprintrin.h>) && !defined(__INTEL_COMPILER)
    // Use x86 general-purpose intrinsics if available and not using Intel compiler
    #include <x86gprintrin.h>
  #else
    // Fallback to standard x86 intrinsics
    #include <x86intrin.h>
  #endif

#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
#if defined(__aarch64__)
// arm64
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t rdtsc() noexcept
{
  // System timer of ARMv8 runs at a different frequency than the CPU's.
  // The frequency is fixed, typically in the range 1-50MHz.  It can be
  // read at CNTFRQ special register.  We assume the OS has set up the virtual timer properly.
  int64_t virtual_timer_value;
  __asm__ volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
  return static_cast<uint64_t>(virtual_timer_value);
}
#elif (defined(__ARM_ARCH) && !defined(_MSC_VER))
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t rdtsc() noexcept
{
  // 32-bit ARM: PMCCNTR is a 32-bit cycle counter that wraps within minutes and also assumes
  // the PMCR.D divider is set. After a wrap, RdtscClock computes a hugely negative difference
  // that never triggers a resync, producing timestamps far in the past. Use the steady clock
  // instead, which was already the fallback when user-mode PMU access was unavailable
  return detail::get_steady_time_ns();
}
#elif defined(__riscv)
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t rdtsc() noexcept
{
  uint64_t tsc;
  __asm__ volatile("rdtime %0" : "=r"(tsc));
  return tsc;
}
#elif defined(__loongarch64)
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t rdtsc() noexcept
{
  uint64_t tsc;
  __asm__ volatile("rdtime.d %0,$r0" : "=r"(tsc));
  return tsc;
}
#elif defined(__s390x__)
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t rdtsc() noexcept
{
  uint64_t tsc;
  __asm__ volatile("stck %0" : "=Q"(tsc) : : "cc");
  return tsc;
}
#elif (defined(_M_ARM) || defined(_M_ARM64) || defined(__PPC64__) || defined(__PPC__))
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t rdtsc() noexcept
{
  // soft failover
  return detail::get_steady_time_ns();
}
#else
/**
 * Get the TSC counter
 * @return rdtsc timestamp
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t rdtsc() noexcept { return __rdtsc(); }
#endif

} // namespace detail

QUILL_END_NAMESPACE
