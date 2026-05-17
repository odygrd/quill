/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include <cstdint>

/**
 * This header exists so that code reachable from the lightweight
 * include path (quill/Logger.h, quill/Frontend.h, and anything under
 * quill/core/) can read wall-clock and monotonic time without pulling in <chrono>
 * on the common path.
 *
 * Windows + MSVC: route through the UCRT shims exposed by <xtimec.h>. This is
 * what MSVC STL's std::chrono::system_clock / std::chrono::steady_clock invoke
 * under the hood and avoids pulling <Windows.h> into public headers.
 *
 * Windows + non-MSVC (MinGW, Clang on Windows): UCRT private shims may not be
 * available; just include <chrono> and call the standard clocks. The frontend
 * include path is unaffected on the MSVC and POSIX configurations that the
 * vast majority of users hit.
 */

#if defined(_WIN32)
  #if defined(_MSC_VER)
    #include <xtimec.h>
  #else
    #include <chrono>
  #endif
#else
  #include <time.h>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * Mirrors std::chrono::duration_cast<std::chrono::nanoseconds>(
 *   std::chrono::system_clock::now().time_since_epoch()).count().
 *
 * POSIX: clock_gettime(CLOCK_REALTIME, &tp) — same as libstdc++ chrono.cc and
 * libc++ chrono.cpp. Full nanosecond precision preserved (matches libstdc++;
 * libc++ truncates to microseconds because its system_clock::duration is
 * microseconds, but we want nanoseconds end-to-end). No error check —
 * CLOCK_REALTIME cannot fail in practice (libstdc++ also doesn't check).
 *
 * Windows: _Xtime_get_ticks() — same underlying call as MSVC STL
 * (GetSystemTimePreciseAsFileTime). It returns 100ns ticks since the Unix
 * epoch (it internally subtracts the 1601→1970 file-time offset). We
 * multiply by 100 to convert to nanoseconds.
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t get_system_time_ns() noexcept
{
#if defined(_WIN32)
  #if defined(_MSC_VER)
  return static_cast<uint64_t>(::_Xtime_get_ticks()) * 100ull;
  #else
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count());
  #endif
#else
  struct timespec tp;
  ::clock_gettime(CLOCK_REALTIME, &tp);
  return static_cast<uint64_t>(tp.tv_sec) * 1'000'000'000ull + static_cast<uint64_t>(tp.tv_nsec);
#endif
}

/**
 * Mirrors std::chrono::duration_cast<std::chrono::nanoseconds>(
 *   std::chrono::steady_clock::now().time_since_epoch()).count().
 *
 * POSIX: clock_gettime(CLOCK_MONOTONIC, &tp) — same as libstdc++ chrono.cc.
 * (libc++ on Linux uses CLOCK_MONOTONIC_RAW when available, but that is
 * unadjusted and rarely what callers expect; libstdc++'s CLOCK_MONOTONIC
 * choice is the standard portable behavior and what most apps assume for
 * std::chrono::steady_clock.)
 *
 * Windows: QueryPerformanceCounter() + QueryPerformanceFrequency() — same
 * as MSVC STL _Query_perf_counter() / _Query_perf_frequency(). The frequency
 * is fixed after system boot, so we cache it in a function-local static.
 * Conversion to nanoseconds uses the same (ticks / freq) * 1e9 scaling the
 * STL applies, split into whole-second and remainder parts to avoid overflow.
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline uint64_t get_steady_time_ns() noexcept
{
#if defined(_WIN32)
  #if defined(_MSC_VER)
  static int64_t const qpc_freq = []() noexcept { return ::_Query_perf_frequency(); }();

  int64_t const ticks = ::_Query_perf_counter();
  int64_t const whole = (ticks / qpc_freq) * 1'000'000'000ll;
  int64_t const part = (ticks % qpc_freq) * 1'000'000'000ll / qpc_freq;
  return static_cast<uint64_t>(whole + part);
  #else
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                 std::chrono::steady_clock::now().time_since_epoch())
                                 .count());
  #endif
#elif defined(__APPLE__)
  // libc++ on Apple uses CLOCK_MONOTONIC_RAW for steady_clock
  struct timespec tp;
  ::clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
  return static_cast<uint64_t>(tp.tv_sec) * 1'000'000'000ull + static_cast<uint64_t>(tp.tv_nsec);
#else
  struct timespec tp;
  ::clock_gettime(CLOCK_MONOTONIC, &tp);
  return static_cast<uint64_t>(tp.tv_sec) * 1'000'000'000ull + static_cast<uint64_t>(tp.tv_nsec);
#endif
}
} // namespace detail

QUILL_END_NAMESPACE
