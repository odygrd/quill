/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <system_error>

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <windows.h>

  #include "quill/core/ThreadUtilities.h"
#elif defined(__APPLE__)
  #include <mach/thread_act.h>
  #include <mach/thread_policy.h>
  #include <pthread.h>
  #include <unistd.h>
#elif defined(__CYGWIN__)
  #include <sched.h>
  #include <unistd.h>
#elif defined(__linux__)
  #include <pthread.h>
  #include <sched.h>
  #include <unistd.h>
#elif defined(__NetBSD__)
  #include <sched.h>
  #include <unistd.h>
#elif defined(__FreeBSD__)
  #include <sched.h>
  #include <unistd.h>
#elif defined(__DragonFly__)
  #include <sched.h>
  #include <unistd.h>
#else
  #include <sched.h>
  #include <unistd.h>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/***/
QUILL_ATTRIBUTE_COLD inline void set_cpu_affinity(uint16_t cpu_id)
{
#if defined(__CYGWIN__)
  // setting cpu affinity on cygwin is not supported
#elif defined(_WIN32)
  // core number starts from 0
  auto const mask = (static_cast<DWORD_PTR>(1) << cpu_id);
  auto ret = SetThreadAffinityMask(GetCurrentThread(), mask);
  if (ret == 0)
  {
    auto const last_error = std::error_code(GetLastError(), std::system_category());

    QUILL_THROW(
      QuillError{std::string{"Failed to set cpu affinity - errno: " + std::to_string(last_error.value()) +
                             " error: " + last_error.message()}});
  }
#elif defined(__APPLE__)
  // I don't think that's possible to link a thread with a specific core with Mac OS X
  // This may be used to express affinity relationships  between threads in the task.
  // Threads with the same affinity tag will be scheduled to share an L2 cache if possible.
  thread_affinity_policy_data_t policy = {cpu_id};

  // Get the mach thread bound to this thread
  thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());

  thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1);
#else
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id, &cpuset);

  auto const err = sched_setaffinity(0, sizeof(cpuset), &cpuset);

  if (QUILL_UNLIKELY(err == -1))
  {
    QUILL_THROW(QuillError{std::string{"Failed to set cpu affinity - errno: " + std::to_string(errno) +
                                       " error: " + strerror(errno)}});
  }
#endif
}

/***/
QUILL_ATTRIBUTE_COLD inline void set_thread_name(char const* name)
{
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(QUILL_NO_THREAD_NAME_SUPPORT)
  // Disabled on MINGW / Cygwin.
  (void)name;
#elif defined(_WIN32)
  std::wstring name_ws = s2ws(name);
  // Set the thread name

  typedef HRESULT(WINAPI * SetThreadDescriptionSignature)(HANDLE, PCWSTR);
  HRESULT hr = callRunTimeDynamicLinkedFunction<HRESULT, SetThreadDescriptionSignature>(
    "KernelBase.dll", "SetThreadDescription", GetCurrentThread(), name_ws.data());

  if (FAILED(hr))
  {
    QUILL_THROW(QuillError{"Failed to set thread name"});
  }
#elif defined(__APPLE__)
  // Apple
  auto const res = pthread_setname_np(name);
  if (res != 0)
  {
    QUILL_THROW(QuillError{std::string{"Failed to set thread name - errno: " + std::to_string(errno) +
                                       " error: " + strerror(errno)}});
  }
#else
  // linux
  char truncated_name[16];
  std::strncpy(truncated_name, name, 15);
  truncated_name[15] = '\0';

  auto const res = pthread_setname_np(pthread_self(), name);
  if (res != 0)
  {
    QUILL_THROW(QuillError{std::string{"Failed to set thread name - errno: " + std::to_string(errno) +
                                       " error: " + strerror(errno)}});
  }
#endif
}

/***/
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline uint32_t get_process_id() noexcept
{
#if defined(__CYGWIN__)
  // get pid on cygwin not supported
  return 0;
#elif defined(_WIN32)
  return static_cast<uint32_t>(GetCurrentProcessId());
#else
  return static_cast<uint32_t>(getpid());
#endif
}
} // namespace detail

QUILL_END_NAMESPACE