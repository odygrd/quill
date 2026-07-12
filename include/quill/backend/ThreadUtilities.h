/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <windows.h>

#elif defined(__APPLE__)
  #include <mach/thread_act.h>
  #include <mach/thread_policy.h>
  #include <pthread.h>
#elif defined(__NetBSD__)
  #include <lwp.h>
  #include <pthread.h>
  #include <unistd.h>
#elif defined(__FreeBSD__)
  #include <pthread_np.h>
  #include <sys/thr.h>
  #include <unistd.h>
#elif defined(__DragonFly__)
  #include <pthread_np.h>
  #include <sys/lwp.h>
  #include <unistd.h>
#elif defined(__OpenBSD__)
  #include <pthread_np.h>
  #include <unistd.h>
#else
  // linux
  #include <pthread.h>
  #include <sys/syscall.h>
  #include <unistd.h>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
#if defined(_WIN32)

/**
 * Convert a string to wstring
 * @param str input string
 * @return the value of input string as wide string
 */
QUILL_NODISCARD inline std::wstring s2ws(std::string const& str)
{
  if (str.empty())
  {
    return std::wstring{};
  }

  int const size_needed =
    ::MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

  if (size_needed == 0)
  {
    return std::wstring{};
  }

  std::wstring ret_val(static_cast<size_t>(size_needed), 0);
  ::MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &ret_val[0], size_needed);
  return ret_val;
}

/**
 * wstring to string
 * @param wstr input wide string
 * @return the value of input wide string as string
 */
QUILL_NODISCARD inline std::string ws2s(std::wstring const& wstr)
{
  if (wstr.empty())
  {
    return std::string{};
  }

  int const size_needed = ::WideCharToMultiByte(
    CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);

  if (size_needed == 0)
  {
    return std::string{};
  }

  std::string ret_val(static_cast<size_t>(size_needed), 0);
  ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &ret_val[0],
                        size_needed, nullptr, nullptr);
  return ret_val;
}

/***/
template <typename ReturnT, typename Signature, typename... Args>
ReturnT callRunTimeDynamicLinkedFunction(std::string const& dll_name,
                                         std::string const& function_name, Args... args)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreaddescription
  // Windows Server 2016, Windows 10 LTSB 2016 and Windows 10 version 1607:
  // GetThreadDescription is only available by Run Time Dynamic Linking in KernelBase.dll.
  // Use GetModuleHandleA since the target DLLs (e.g. KernelBase.dll) are always loaded.
  HMODULE const hinstLibrary = ::GetModuleHandleA(dll_name.c_str());

  if (QUILL_UNLIKELY(hinstLibrary == nullptr))
  {
    QUILL_THROW(QuillError{std::string{"Failed to get module handle for " + dll_name}});
  }

  // Using a C-style cast or memcpy to avoid Clang's strict function pointer casting rules
  FARPROC proc_address = GetProcAddress(hinstLibrary, function_name.c_str());

  if (QUILL_UNLIKELY(proc_address == nullptr))
  {
    QUILL_THROW(QuillError{std::string{"Failed to call " + function_name + " " + dll_name}});
  }

  // Use memcpy to avoid the strict cast warning in Clang
  Signature callable;
  std::memcpy(&callable, &proc_address, sizeof(proc_address));

  return callable(static_cast<Args&&>(args)...);
}
#endif

/**
 * Returns the name of the thread. By default, each thread is unnamed.
 * If set_thread_name has not been used to set the name of the specified thread,
 * a null string is retrieved into name.
 * @return the thread name
 */
QUILL_NODISCARD QUILL_EXPORT QUILL_ATTRIBUTE_USED inline std::string get_thread_name()
{
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__ANDROID__) || \
  defined(QUILL_NO_THREAD_NAME_SUPPORT)
  // Disabled on MINGW / Cygwin.
  return std::string{"ThreadNameDisabled"};
#elif defined(_WIN32)
  PWSTR data = nullptr;
  typedef HRESULT(WINAPI * GetThreadDescriptionSignature)(HANDLE, PWSTR*);

  // KernelBase.dll is always loaded in every Windows process
  HMODULE const hinstLibrary = ::GetModuleHandleW(L"KernelBase.dll");

  if (!hinstLibrary)
  {
    return std::string{"ThreadNameDisabled"};
  }

  FARPROC const proc_address = GetProcAddress(hinstLibrary, "GetThreadDescription");

  if (!proc_address)
  {
    return std::string{"ThreadNameDisabled"};
  }

  GetThreadDescriptionSignature callable;
  std::memcpy(&callable, &proc_address, sizeof(proc_address));

  HRESULT const hr = callable(GetCurrentThread(), &data);

  if (FAILED(hr) || QUILL_UNLIKELY(data == nullptr))
  {
    return std::string{"ThreadNameDisabled"};
  }

  std::wstring const wide_name{data, wcsnlen_s(data, 256)};
  LocalFree(data);
  return ws2s(wide_name);
#else
  // Apple, linux
  char thread_name[16] = {'\0'};
  #if defined(__OpenBSD__) || defined(__FreeBSD__)
  pthread_get_name_np(pthread_self(), &thread_name[0], 16);
  #else
  auto res = pthread_getname_np(pthread_self(), &thread_name[0], 16);
  if (res != 0)
  {
    // This runs on each thread's first log statement, and glibc reads the name from procfs;
    // degrade gracefully like the Windows branch instead of failing the log statement in
    // /proc-less environments (minimal containers, chroots)
    return std::string{"ThreadNameDisabled"};
  }
  #endif
  return std::string{&thread_name[0], strlen(&thread_name[0])};
#endif
}

#if defined(QUILL_USE_SEQUENTIAL_THREAD_ID)
extern std::atomic<uint32_t> g_next_thread_id;
extern QUILL_THREAD_LOCAL uint32_t g_cached_thread_id;
#endif

/**
 * Returns the os assigned ID of the thread
 * @return the thread ID of the calling thread
 */
QUILL_NODISCARD QUILL_EXPORT QUILL_ATTRIBUTE_USED inline uint32_t get_thread_id() noexcept
{
#if defined(QUILL_USE_SEQUENTIAL_THREAD_ID)
  if (QUILL_LIKELY(g_cached_thread_id != 0u))
  {
    return g_cached_thread_id;
  }

  g_cached_thread_id = g_next_thread_id.fetch_add(1u, std::memory_order_relaxed) + 1u;
  return g_cached_thread_id;
#elif defined(__CYGWIN__)
  // get thread id on cygwin not supported
  return 0;
#elif defined(_WIN32)
  return static_cast<uint32_t>(GetCurrentThreadId());
#elif defined(__linux__)
  return static_cast<uint32_t>(::syscall(SYS_gettid));
#elif defined(__APPLE__)
  uint64_t tid64;
  pthread_threadid_np(nullptr, &tid64);
  return static_cast<uint32_t>(tid64);
#elif defined(__NetBSD__)
  return static_cast<uint32_t>(_lwp_self());
#elif defined(__FreeBSD__)
  long lwpid;
  thr_self(&lwpid);
  return static_cast<uint32_t>(lwpid);
#elif defined(__DragonFly__)
  return static_cast<uint32_t>(lwp_gettid());
#elif defined(__OpenBSD__)
  return static_cast<uint32_t>(getthrid());
#else
  return reinterpret_cast<uintptr_t>(pthread_self()); // (Ab)use pthread_self as a last resort option
#endif
}

} // namespace detail

QUILL_END_NAMESPACE
