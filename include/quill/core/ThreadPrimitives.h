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

/**
 * Mirrors std::this_thread::sleep_for(std::chrono::nanoseconds{ns}).
 */
QUILL_ATTRIBUTE_HOT inline void sleep_for_ns(uint64_t ns) noexcept
{
  if (ns == 0)
  {
    return;
  }

#if defined(_WIN32)
  // ceil(ns / 1'000'000) and clamp to ULONG_MAX
  uint64_t ms = (ns + 999'999ull) / 1'000'000ull;
  if (ms > 0xFFFFFFFFull)
  {
    ms = 0xFFFFFFFFull;
  }
  ::Sleep(static_cast<unsigned long>(ms));
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
QUILL_ATTRIBUTE_HOT inline void yield_thread() noexcept
{
#if defined(_WIN32)
  ::SwitchToThread();
#else
  ::sched_yield();
#endif
}
} // namespace detail

QUILL_END_NAMESPACE
