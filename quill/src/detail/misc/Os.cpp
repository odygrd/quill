#include "quill/detail/misc/Os.h"
#include "quill/QuillError.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/Utilities.h"
#include <array>
#include <cerrno> // for errno, EINVAL, ENOMEM
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <string>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <io.h>
  #include <malloc.h>
  #include <share.h>
  #include <windows.h>
  #include <processthreadsapi.h>
#elif defined(__APPLE__)
  #include <mach/thread_act.h>
  #include <mach/thread_policy.h>
  #include <pthread.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <sys/sysctl.h>
  #include <sys/types.h>
  #include <unistd.h>
#elif defined(__CYGWIN__)
  #include <sched.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <unistd.h>
#elif defined(__linux__)
  #include <sched.h>
  #include <sys/mman.h>
  #include <sys/prctl.h>
  #include <sys/stat.h>
  #include <syscall.h>
  #include <unistd.h>
#endif

namespace quill
{
namespace detail
{
#if defined(_WIN32)
/***/
size_t get_wide_string_encoding_size(std::wstring_view s)
{
  return static_cast<size_t>(::WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr));
}

/***/
void wide_string_to_narrow(void* dest, size_t required_bytes, std::wstring_view s) 
{
  ::WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                        reinterpret_cast<char*>(dest), static_cast<int>(required_bytes), NULL, NULL);
}
#endif

/***/
tm* gmtime_rs(time_t const* timer, tm* buf)
{
#if defined(_WIN32)
  errno_t const res = gmtime_s(buf, timer);
  if (res)
  {
    std::ostringstream error_msg;
    error_msg << "failed to call gmtime_rs, with error message "
              << "errno : \"" << res << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
  return buf;
#else
  tm* res = gmtime_r(timer, buf);
  if (QUILL_UNLIKELY(!res))
  {
    std::ostringstream error_msg;
    error_msg << "failed to call gmtime_rs, with error message "
              << "\"" << strerror(errno) << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
  return res;
#endif
}

/***/
tm* localtime_rs(time_t const* timer, tm* buf)
{
#if defined(_WIN32)
  errno_t const res = localtime_s(buf, timer);
  if (res)
  {
    std::ostringstream error_msg;
    error_msg << "failed to call localtime_rs, with error message "
              << "errno: \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
  return buf;
#else
  tm* res = localtime_r(timer, buf);
  if (QUILL_UNLIKELY(!res))
  {
    std::ostringstream error_msg;
    error_msg << "failed to call localtime_rs, with error message "
              << "\"" << strerror(errno) << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
  return res;
#endif
}

/***/
void set_cpu_affinity(uint16_t cpu_id)
{
#if defined(__CYGWIN__)
  // setting cpu affinity on cygwin is not supported
#elif defined(_WIN32)
  // core number starts from 0
  auto mask = (static_cast<DWORD_PTR>(1) << cpu_id);
  auto ret = SetThreadAffinityMask(GetCurrentThread(), mask);
  if (ret == 0)
  {
    auto const last_error = std::error_code(GetLastError(), std::system_category());

    std::ostringstream error_msg;
    error_msg << "failed to call set_cpu_affinity, with error message "
              << "\"" << last_error.message() << "\", errno \"" << last_error.value() << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
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
    std::ostringstream error_msg;
    error_msg << "failed to call set_cpu_affinity, with error message "
              << "\"" << strerror(errno) << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
#endif
}

/***/
void set_thread_name(char const* name)
{
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(QUILL_NO_THREAD_NAME_SUPPORT)
  // Disabled on MINGW / Cygwin.
  (void)name;
#elif defined(_WIN32)
  std::wstring name_ws = s2ws(name);
  // Set the thread name
  HRESULT hr = SetThreadDescription(GetCurrentThread(), name_ws.data());
  if (FAILED(hr))
  {
    QUILL_THROW(QuillError{"Failed to set thread name"});
  }
#elif defined(__APPLE__)
  auto const res = pthread_setname_np(name);
  if (res != 0)
  {
    QUILL_THROW(QuillError{"Failed to set thread name. error: " + std::to_string(res)});
  }
#else
  auto const err = prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name), 0, 0, 0);

  if (QUILL_UNLIKELY(err == -1))
  {
    std::ostringstream error_msg;
    error_msg << "failed to call set_thread_name, with error message "
              << "\"" << strerror(errno) << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
#endif
}

/***/
std::string get_thread_name()
{
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(QUILL_NO_THREAD_NAME_SUPPORT)
  // Disabled on MINGW / Cygwin.
  return std::string{};
#elif defined(_WIN32)
  PWSTR data;
  HRESULT hr = GetThreadDescription(GetCurrentThread(), &data);
  if (FAILED(hr))
  {
    QUILL_THROW(QuillError{"Failed to get thread name"});
  }

  std::wstring tname{&data[0], wcslen(&data[0])};
  LocalFree(data);
  return ws2s(tname);
#else
  // Apple, linux
  std::array<char, 16> thread_name{'\0'};
  pthread_t thread = pthread_self();
  auto res = pthread_getname_np(thread, &thread_name[0], 16);
  if (res != 0)
  {
    QUILL_THROW(QuillError{"Failed to get thread name. error: " + std::to_string(res)});
  }
  return std::string{&thread_name[0], strlen(&thread_name[0])};
#endif
}

/***/
uint32_t get_thread_id() noexcept
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
#endif
}

/***/
uint32_t get_process_id() noexcept
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

/***/
size_t get_page_size() noexcept
{
  // thread local to avoid race condition when more than one threads are creating the queue at the same time
  static thread_local uint32_t page_size{0};
  if (page_size == 0)
  {
#if defined(__CYGWIN__)
    page_size = 4096;
#elif defined(_WIN32)
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    page_size = std::max(system_info.dwPageSize, system_info.dwAllocationGranularity);
#else
    page_size = static_cast<uint32_t>(sysconf(_SC_PAGESIZE));
#endif
  }
  return page_size;
}

/***/
void* aligned_alloc(size_t alignment, size_t size)
{
#if defined(_WIN32)
  void* p = _aligned_malloc(size, alignment);

  if (!p)
  {
    std::ostringstream error_msg;
    error_msg << "aligned_alloc failed with error message "
              << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  return p;
#else
  void* ret = nullptr;

  auto res = posix_memalign(&ret, alignment, size);
  if (QUILL_UNLIKELY(res == EINVAL || res == ENOMEM))
  {
    std::ostringstream error_msg;
    error_msg << "aligned_alloc failed with error message "
              << "\"" << strerror(res) << "\", errno \"" << res << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  return ret;
#endif
}

/***/
void aligned_free(void* ptr) noexcept
{
#if defined(_WIN32)
  _aligned_free(ptr);
#else
  free(ptr);
#endif
}

/***/
time_t timegm(tm* tm)
{
#if defined(_WIN32)
  time_t const ret_val = ::_mkgmtime(tm);

  if (QUILL_UNLIKELY(ret_val == -1))
  {
    QUILL_THROW(QuillError{"_mkgmtime failed."});
  }

  return ret_val;
#else
  time_t const ret_val = ::timegm(tm);

  if (QUILL_UNLIKELY(ret_val == (time_t)-1))
  {
    QUILL_THROW(QuillError{"timegm failed."});
  }

  return ret_val;
#endif
}

/***/
bool is_colour_terminal() noexcept
{
#if defined(_WIN32)
  return true;
#else
  // Get term from env
  auto* env_p = std::getenv("TERM");

  if (env_p == nullptr)
  {
    return false;
  }

  static constexpr std::array<char const*, 14> terms = {
    {"ansi", "color", "console", "cygwin", "gnome", "konsole", "kterm", "linux", "msys", "putty",
     "rxvt", "screen", "vt100", "xterm"}};

  return std::any_of(terms.begin(), terms.end(),
                     [&](char const* term) { return std::strstr(env_p, term) != nullptr; });
#endif
}

/***/
bool is_in_terminal(FILE* file) noexcept
{
#if defined(_WIN32)
  bool const is_atty = ::_isatty(_fileno(file)) != 0;

  // ::GetConsoleMode() should return 0 if file is redirected or does not point to the actual console
  DWORD console_mode;
  bool const is_console =
    ::GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file))), &console_mode) != 0;

  return is_atty && is_console;
#else
  return ::isatty(fileno(file)) != 0;
#endif
}

} // namespace detail
} // namespace quill
