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
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <windows.h>

  #include <fileapi.h>
  #include <io.h>
  #include <malloc.h>
  #include <share.h>
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
  #include <pthread.h>
  #include <sched.h>
  #include <sys/mman.h>
  #include <sys/prctl.h>
  #include <sys/stat.h>
  #include <syscall.h>
  #include <unistd.h>
#endif

#include "quill/detail/misc/Utilities.h"

namespace quill::detail
{
#if defined(_WIN32)
/***/
size_t get_wide_string_encoding_size(std::wstring_view s)
{
  return static_cast<size_t>(WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                                                   nullptr, 0, nullptr, nullptr));
}

/***/
void wide_string_to_narrow(void* dest, size_t required_bytes, std::wstring_view s)
{
  WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                        static_cast<char*>(dest), static_cast<int>(required_bytes), nullptr, nullptr);
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
  auto const res = localtime_s(buf, timer);
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

#if defined(_WIN32)
/***/
template <typename ReturnT, typename Signature, typename... Args>
ReturnT callRunTimeDynamicLinkedFunction(const std::string& dll_name,
                                         const std::string& function_name, Args... args)
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
    std::ostringstream error_msg;
    error_msg << "Failed to load library "
              << "\"" << dll_name << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  auto const callable = reinterpret_cast<Signature>(GetProcAddress(hinstLibrary, function_name.c_str()));

  if (QUILL_UNLIKELY(callable == nullptr))
  {
    FreeLibrary(hinstLibrary);
    std::ostringstream error_msg;
    error_msg << "Failed to call "
              << "\"" << function_name << "\" in "
              << "\"" << dll_name << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  ReturnT const hr = callable(std::forward<Args>(args)...);
  BOOL const fFreeResult = FreeLibrary(hinstLibrary);

  if (QUILL_UNLIKELY(!fFreeResult))
  {
    std::ostringstream error_msg;
    error_msg << "Failed to free library "
              << "\"" << dll_name << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  return hr;
}
#endif

/***/
void set_cpu_affinity(uint16_t cpu_id)
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
    std::ostringstream error_msg;
    error_msg << "failed to call set_thread_name, with error message "
              << "\"" << strerror(errno) << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }
#else
  // linux
  char truncated_name[16];
  std::strncpy(truncated_name, name, 15);
  truncated_name[15] = '\0';

  auto const res = pthread_setname_np(pthread_self(), truncated_name);
  if (res != 0)
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
  std::array<char, 16> thread_name{'\0'};
  auto res = pthread_getname_np(pthread_self(), &thread_name[0], 16);
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
void* alloc_aligned(size_t size, size_t alignment, bool huge_pages /* = false */)
{
#if defined(_WIN32)
  void* p = _aligned_malloc(size, alignment);

  if (!p)
  {
    std::ostringstream error_msg;
    error_msg << "alloc_aligned failed with error message "
              << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  return p;
#else
  // Calculate the total size including the metadata and alignment
  constexpr size_t metadata_size{2u * sizeof(size_t)};
  size_t const total_size{size + metadata_size + alignment};

  // Allocate the memory
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;

  #if defined(__linux__)
  if (huge_pages)
  {
    flags |= MAP_HUGETLB;
  }
  #endif

  void* mem = ::mmap(nullptr, total_size, PROT_READ | PROT_WRITE, flags, -1, 0);

  if (mem == MAP_FAILED)
  {
    std::ostringstream error_msg;
    error_msg << "mmap failed with error message "
              << "\"" << strerror(errno) << "\", errno \"" << errno << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  // Calculate the aligned address after the metadata
  std::byte* aligned_address =
    detail::align_pointer<std::byte>(static_cast<std::byte*>(mem) + metadata_size, alignment);

  // Calculate the offset from the original memory location
  auto const offset = static_cast<size_t>(aligned_address - static_cast<std::byte*>(mem));

  // Store the size and offset information in the metadata
  std::memcpy(aligned_address - sizeof(size_t), &total_size, sizeof(total_size));
  std::memcpy(aligned_address - (2u * sizeof(size_t)), &offset, sizeof(offset));

  return aligned_address;
#endif
}

/***/
void free_aligned(void* ptr) noexcept
{
#if defined(_WIN32)
  _aligned_free(ptr);
#else
  // Retrieve the size and offset information from the metadata
  size_t offset;
  std::memcpy(&offset, static_cast<std::byte*>(ptr) - (2u * sizeof(size_t)), sizeof(offset));

  size_t total_size;
  std::memcpy(&total_size, static_cast<std::byte*>(ptr) - sizeof(size_t), sizeof(total_size));

  // Calculate the original memory block address
  void* mem = static_cast<std::byte*>(ptr) - offset;

  ::munmap(mem, total_size);
#endif
}

/***/
time_t timegm(tm* tm)
{
#if defined(_WIN32)
  time_t const ret_val = _mkgmtime(tm);

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
  bool const is_atty = _isatty(_fileno(file)) != 0;

  // ::GetConsoleMode() should return 0 if file is redirected or does not point to the actual console
  DWORD console_mode;
  bool const is_console =
    GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file))), &console_mode) != 0;

  return is_atty && is_console;
#else
  return ::isatty(fileno(file)) != 0;
#endif
}

/***/
bool fsync(FILE* fd)
{
#ifdef _WIN32
  return FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd)))) != 0;
#else
  return ::fsync(fileno(fd)) == 0;
#endif
}
} // namespace quill::detail
