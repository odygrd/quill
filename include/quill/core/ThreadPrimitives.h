/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/ChronoTimeUtils.h"
#ifndef QUILL_MODULE
  #include <cstdint>
#endif

/**
 * This header exists so that code reachable from the lightweight
 * include path (quill/Logger.h, quill/Frontend.h, and anything under
 * quill/core/) can sleep and yield without pulling in <thread>
 */

#if defined(_WIN32)
// Avoid <windows.h>. Forward-declare the two Win32 functions we need.
// MSVC STL itself avoids <windows.h> here (it routes through UCRT _Thrd_*
// shims); we go directly to Win32 so we don't depend on UCRT internals,
// but the resulting kernel calls (Sleep, SwitchToThread) are identical to
// what MSVC STL invokes.
extern "C"
{
  __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
  __declspec(dllimport) int __stdcall SwitchToThread(void);
}
#else
  #include <errno.h>
  #include <sched.h>
  #include <time.h>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{

#if defined(_WIN32)
QUILL_NODISCARD constexpr unsigned long sleep_timeout_ms(uint64_t ns) noexcept
{
  uint64_t const milliseconds = (ns / 1'000'000ull) + static_cast<uint64_t>((ns % 1'000'000ull) != 0);
  return static_cast<unsigned long>((milliseconds > 0xFFFFFFFFull) ? 0xFFFFFFFFull : milliseconds);
}
#endif

/**
 * Mirrors std::this_thread::sleep_for(std::chrono::nanoseconds{ns}).
 *
 * On Windows, a positive request below 1 ms yields and checks a steady-clock deadline.
 */
inline void sleep_for_ns(uint64_t ns) noexcept
{
  if (ns == 0)
  {
    return;
  }

#if defined(_WIN32)
  if (ns < 1'000'000ull)
  {
    uint64_t const deadline = get_steady_time_ns() + ns;
    do
    {
      ::SwitchToThread();
    } while (get_steady_time_ns() < deadline);
    return;
  }

  ::Sleep(sleep_timeout_ms(ns));
#else
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(ns / 1'000'000'000ull);
  ts.tv_nsec = static_cast<long>(ns % 1'000'000'000ull);
  while (::nanosleep(&ts, &ts) == -1 && errno == EINTR)
  {
  }
#endif
}

/**
 * Mirrors std::this_thread::yield().
 */
inline void yield_thread() noexcept
{
#if defined(_WIN32)
  ::SwitchToThread();
#else
  ::sched_yield();
#endif
}

/**
 * Waits before checking again for work the backend usually completes within microseconds.
 * Yields for the first attempts, then requests a sleep of `sleep_duration_ns`, or keeps yielding
 * when it is zero. On Windows the sleep duration is rounded up to whole milliseconds before
 * calling Sleep() so the waiting thread can give up its core.
 */
inline void backoff_wait(uint32_t& attempt, uint64_t sleep_duration_ns) noexcept
{
  constexpr uint32_t yields_before_sleeping{64};

  if ((sleep_duration_ns == 0) || (attempt < yields_before_sleeping))
  {
    attempt += (attempt < yields_before_sleeping) ? 1u : 0u;
    yield_thread();
  }
  else
  {
#if defined(_WIN32)
    // Not sleep_for_ns(), which yields for a wait below 1 ms
    ::Sleep(sleep_timeout_ms(sleep_duration_ns));
#else
    sleep_for_ns(sleep_duration_ns);
#endif
  }
}
} // namespace detail

QUILL_END_NAMESPACE
