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

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <windows.h>

  #include <codecvt>
  #include <locale>
#elif defined(__APPLE__)
  #include <mach/thread_act.h>
  #include <mach/thread_policy.h>
  #include <pthread.h>
#elif defined(__linux__)
  #include <pthread.h>
  #include <sys/syscall.h>
  #include <unistd.h>
#elif defined(__NetBSD__)
  #include <lwp.h>
  #include <unistd.h>
#elif defined(__FreeBSD__)
  #include <sys/thr.h>
  #include <unistd.h>
#elif defined(__DragonFly__)
  #include <sys/lwp.h>
  #include <unistd.h>
#endif

namespace quill::detail
{
#if defined(_WIN32)

/**
 * Convert a string to wstring
 * @param str input string
 * @return the value of input string as wide string
 */
QUILL_NODISCARD inline std::wstring s2ws(std::string const& str) noexcept
{
  using convert_t = std::codecvt_utf8_utf16<wchar_t>;
  std::wstring_convert<convert_t, wchar_t> converter;
  return converter.from_bytes(str);
}

/**
 * wstring to string
 * @param wstr input wide string
 * @return the value of input wide string as string
 */
QUILL_NODISCARD inline std::string ws2s(std::wstring const& wstr) noexcept
{
  using convert_t = std::codecvt_utf8_utf16<wchar_t>;
  std::wstring_convert<convert_t, wchar_t> converter;
  return converter.to_bytes(wstr);
}

/***/
template <typename ReturnT, typename Signature, typename... Args>
ReturnT callRunTimeDynamicLinkedFunction(std::string const& dll_name,
                                         std::string const& function_name, Args... args)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreaddescription
  // Windows Server 2016, Windows 10 LTSB 2016 and Windows 10 version 1607: e.g. GetThreadDescription is only available by Run Time Dynamic Linking in KernelBase.dll.

  #ifdef UNICODE
  HINSTANCE const hinstLibrary = LoadLibraryW(s2ws(dll_name).c_str());
  #else
  HINSTANCE const hinstLibrary = LoadLibraryA(dll_name.c_str());
  #endif

  if (QUILL_UNLIKELY(hinstLibrary == nullptr))
  {
    QUILL_THROW(QuillError{std::string{"Failed to load library " + dll_name}});
  }

  auto const callable = reinterpret_cast<Signature>(GetProcAddress(hinstLibrary, function_name.c_str()));

  if (QUILL_UNLIKELY(callable == nullptr))
  {
    FreeLibrary(hinstLibrary);
    QUILL_THROW(QuillError{std::string{"Failed to call " + function_name + " " + dll_name}});
  }

  ReturnT const hr = callable(static_cast<Args&&>(args)...);
  BOOL const fFreeResult = FreeLibrary(hinstLibrary);

  if (QUILL_UNLIKELY(!fFreeResult))
  {
    QUILL_THROW(QuillError{std::string{"Failed to free library " + dll_name}});
  }

  return hr;
}
#endif

/**
 * Returns the name of the thread. By default, each thread is unnamed.
 * If set_thread_name has not been used to set the name of the specified thread,
 * a null string is retrieved into name.
 * @return the thread name
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline std::string get_thread_name()
{
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(QUILL_NO_THREAD_NAME_SUPPORT)
  // Disabled on MINGW / Cygwin.
  return std::string{"ThreadNameDisabled"};
#elif defined(_WIN32)
  PWSTR data = nullptr;

  typedef HRESULT(WINAPI * GetThreadDescriptionSignature)(HANDLE, PWSTR*);
  HRESULT hr = callRunTimeDynamicLinkedFunction<HRESULT, GetThreadDescriptionSignature>(
    "KernelBase.dll", "GetThreadDescription", GetCurrentThread(), &data);

  if (FAILED(hr))
  {
    QUILL_THROW(QuillError{"Failed to get thread name"});
  }

  if (QUILL_UNLIKELY(data == nullptr))
  {
    QUILL_THROW(QuillError{"Failed to get thread name. Invalid data."});
  }

  const std::wstring tname{&data[0], wcslen(&data[0])};
  LocalFree(data);
  return ws2s(tname);
#else
  // Apple, linux
  char thread_name[16] = {'\0'};
  auto res = pthread_getname_np(pthread_self(), &thread_name[0], 16);
  if (res != 0)
  {
    QUILL_THROW(QuillError{"Failed to get thread name. error: " + std::to_string(res)});
  }
  return std::string{&thread_name[0], strlen(&thread_name[0])};
#endif
}

/**
 * Returns the os assigned ID of the thread
 * @return the thread ID of the calling thread
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline uint32_t get_thread_id() noexcept
{
#if defined(__CYGWIN__)
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
#else
  return reinterpret_cast<uintptr_t>(pthread_self()); // (Ab)use pthread_self as a last resort option
#endif
}
} // namespace quill::detail