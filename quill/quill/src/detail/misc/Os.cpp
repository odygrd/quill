#include "quill/detail/misc/Os.h"

#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/Utilities.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <sys/stat.h>
#include <system_error>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <malloc.h>
  #include <io.h>
  #include <windows.h>
  #include <share.h>

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
#elif defined(__linux__)
  #include <sched.h>
  #include <sys/mman.h>
  #include <sys/prctl.h>
  #include <sys/syscall.h>
  #include <unistd.h>

  /**
   * Detect if _MAP_POPULATE is available for mmap
   */
  #include <linux/version.h>
  #if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 22)
    #define _MAP_POPULATE_AVAILABLE
  #endif
#endif

/**
 * MMAP Flags for linux/Macos only
 */
#ifdef _MAP_POPULATE_AVAILABLE
  #define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE)
#else
  #define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#endif

namespace quill
{
namespace detail
{

/***/
tm* gmtime_rs(time_t const* timer, tm* buf)
{
#if defined(_WIN32)
  errno_t const res = gmtime_s(buf, timer);
  if (res)
  {
    throw std::system_error(res, std::system_category());
  }
  return buf;
#else
  tm* res = gmtime_r(timer, buf);
  if (QUILL_UNLIKELY(!res))
  {
    throw std::system_error(errno, std::system_category());
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
    throw std::system_error(res, std::system_category());
  }
  return buf;
#else
  tm* res = localtime_r(timer, buf);
  if (QUILL_UNLIKELY(!res))
  {
    throw std::system_error(errno, std::system_category());
  }
  return res;
#endif
}

/***/
void set_cpu_affinity(uint16_t cpu_id)
{
#if defined(_WIN32)
  // core number starts from 0
  auto mask = (static_cast<DWORD_PTR>(1) << cpu_id);
  auto ret = SetThreadAffinityMask(GetCurrentThread(), mask);
  if (ret == 0)
  {
    throw std::system_error(std::error_code(GetLastError(), std::system_category()));
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
    throw std::system_error(errno, std::system_category());
  }
#endif
}

/***/
void set_thread_name(char const* name)
{
#if defined(__MINGW32__) || defined(__MINGW64__)
  // Disabled on MINGW.
  (void)name;
#elif defined(_WIN32)
  std::wstring name_ws = s2ws(name);
  // Set the thread name
  HRESULT hr = SetThreadDescription(GetCurrentThread(), name_ws.data());
  if (FAILED(hr))
  {
    throw std::runtime_error("Failed to set thread name");
  }
#elif defined(__APPLE__)
  auto const res = pthread_setname_np(name);
  if (res != 0)
  {
    throw std::runtime_error("Failed to set thread name. error: " + std::to_string(res));
  }
#else
  auto const err = prctl(PR_SET_NAME, reinterpret_cast<unsigned long>(name), 0, 0, 0);

  if (QUILL_UNLIKELY(err == -1))
  {
    throw std::system_error(errno, std::system_category());
  }
#endif
}

/***/
uint32_t get_thread_id() noexcept
{
#if defined(_WIN32)
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
size_t get_page_size() noexcept
{
  // thread local to avoid race condition when more than one threads are creating the queue at the same time
  static thread_local uint32_t page_size{0};
#if defined(_WIN32)
  SYSTEM_INFO system_info;
  GetSystemInfo(&system_info);
  page_size = std::max(system_info.dwPageSize, system_info.dwAllocationGranularity);
#else
  page_size = static_cast<uint32_t>(sysconf(_SC_PAGESIZE));
#endif
  return page_size;
}

/***/
void madvice(void* addr, size_t len)
{
#if defined(_WIN32)
  // Nothing to do on windows, silence the warning
  (void)addr;
  (void)len;
#else
  // It looks like madvice hint has no effect but we do it anyway ..
  auto const res = madvise(addr, len, MADV_SEQUENTIAL);
  if (res == -1)
  {
    throw std::system_error(res, std::system_category());
  }
#endif
}

/***/
void* aligned_alloc(size_t alignment, size_t size)
{
#if defined(_WIN32)
  return _aligned_malloc(size, alignment);
#else
  void* ret = nullptr;

  auto res = posix_memalign(&ret, alignment, size);
  if (QUILL_UNLIKELY(res == EINVAL || res == ENOMEM))
  {
    throw std::system_error(res, std::system_category());
  }

  return ret;
#endif
}

/***/
void aligned_free(void* ptr) noexcept
{
#ifdef WIN32
  return _aligned_free(ptr);
#else
  return free(ptr);
#endif
}

/***/
FILE* fopen(filename_t const& filename, std::string const& mode)
{
  FILE* fp{nullptr};
#if defined(_WIN32)
  std::wstring const w_mode = s2ws(mode);
  fp = ::_wfsopen((filename.c_str()), w_mode.data(), _SH_DENYNO);
#else
  fp = ::fopen(filename.data(), mode.data());
#endif
  if (!fp)
  {
    throw std::system_error(errno, std::system_category());
  }
  return fp;
}

/***/
size_t fsize(FILE* file)
{
  if (!file)
  {
    throw std::runtime_error("Can not get the file size. file is nullptr");
  }

#if defined(_WIN32) && !defined(__CYGWIN__)
  auto const fd = ::_fileno(file);
  auto const ret = ::_filelength(fd);

  if (ret >= 0)
  {
    return static_cast<size_t>(ret);
  }
#else
  auto const fd = fileno(file);
  struct stat st;

  if (fstat(fd, &st) == 0)
  {
    return static_cast<size_t>(st.st_size);
  }
#endif

  // failed to get the file size
  throw std::system_error(errno, std::system_category());
}

/***/
int remove(filename_t const& filename) noexcept
{
#if defined(_WIN32)
  return ::_wremove(filename.c_str());
#else
  return std::remove(filename.c_str());
#endif
}

#if defined(_WIN32)
/***/
void wstring_to_utf8(fmt::wmemory_buffer const& w_mem_buffer, fmt::memory_buffer& mem_buffer)
{
  auto bytes_needed = static_cast<int32_t>(mem_buffer.capacity() - mem_buffer.size());

  if ((w_mem_buffer.size() + 1) * 2 > static_cast< size_t >(bytes_needed))
  {
    // if our given string is larger than the capacity, calculate how many bytes we need
    bytes_needed = ::WideCharToMultiByte(
      CP_UTF8, 0, w_mem_buffer.data(), static_cast<int>(w_mem_buffer.size()), NULL, 0, NULL, NULL);
  }

  if (QUILL_UNLIKELY(bytes_needed == 0))
  {
    throw std::system_error(std::error_code(GetLastError(), std::system_category()));
  }

  // convert
  bytes_needed =
    ::WideCharToMultiByte(CP_UTF8, 0, w_mem_buffer.data(), static_cast<int>(w_mem_buffer.size()),
                          mem_buffer.data() + mem_buffer.size(), bytes_needed, NULL, NULL);

  if (QUILL_UNLIKELY(bytes_needed == 0))
  {
    throw std::system_error(std::error_code(GetLastError(), std::system_category()));
  }

  // resize again in case we didn't calculate before how many bytes needed (1st call to WideCharToMultiByte was skipped)
  mem_buffer.resize(static_cast<uint32_t>(bytes_needed + mem_buffer.size()));
}
#endif

/***/
std::pair<unsigned char*, void*> create_memory_mapped_files(size_t capacity)
{
  if (!is_pow_of_two(capacity))
  {
    throw std::runtime_error("capacity needs to be power of two");
  }

  if (capacity % get_page_size() != 0)
  {
    throw std::runtime_error("capacity needs to be multiple of page size");
  }

#if defined(_WIN32)
  HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                      static_cast<DWORD>(capacity * 2), NULL);

  if (!hMapFile)
  {
    throw std::system_error(std::error_code(GetLastError(), std::system_category()));
  }

  for (;;)
  {
    // find a free address space with the correct size
    auto address =
      static_cast<unsigned char*>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, capacity * 2));

    if (!address)
    {
      CloseHandle(hMapFile);
      throw std::system_error(std::error_code(GetLastError(), std::system_category()));
    }

    // found a big enough address space. hopefully it will remain free while we map to it. if not,
    // we'll try again.
    UnmapViewOfFile(address);

    auto addr1 =
      static_cast<unsigned char*>(MapViewOfFileEx(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, capacity, address));

    if (addr1 != address)
    {
      // Try again if it did not remain free
      DWORD err = GetLastError();
      if (err == ERROR_INVALID_ADDRESS)
      {
        continue;
      }
      else
      {
        CloseHandle(hMapFile);
        throw std::system_error(std::error_code(GetLastError(), std::system_category()));
      }
    }

    auto addr2 = static_cast<unsigned char*>(
      MapViewOfFileEx(hMapFile, FILE_MAP_WRITE, 0, 0, capacity, address + capacity));

    if (addr2 != address + capacity)
    {
      // We will try again but first unmap the previous mapped file
      UnmapViewOfFile(addr1);

      DWORD err = GetLastError();
      if (err == ERROR_INVALID_ADDRESS)
      {
        continue;
      }
      else
      {
        CloseHandle(hMapFile);
        throw std::system_error(std::error_code(err, std::system_category()));
      }
    }

    // All okay
    return std::pair<unsigned char*, void*>{address, hMapFile};
  }
#else
  char shm_path[] = "/dev/shm/quill-XXXXXX";
  char tmp_path[] = "/tmp/quill-XXXXXX";
  char const* chosen_path{nullptr};

  // Try to open an fd by creating a unique file in one of the above locations
  int fd = mkstemp(shm_path);

  if (fd < 0)
  {
    // if we failed try the tmp path
    fd = mkstemp(tmp_path);

    if (fd < 0)
    {
      throw std::system_error(errno, std::system_category());
    }

    chosen_path = tmp_path;
  }
  else
  {
    chosen_path = shm_path;
  }

  // Delete the file as we only want the fd
  if (unlink(chosen_path) == -1)
  {
    throw std::system_error(errno, std::system_category());
  }

  if (ftruncate(fd, static_cast<off_t>(capacity)) == -1)
  {
    close(fd);
    throw std::system_error(errno, std::system_category());
  }

  // ask mmap for a good address where we can put both virtual copies of the buffer
  auto address = static_cast<unsigned char*>(mmap(nullptr, 2 * capacity, PROT_NONE, MMAP_FLAGS, -1, 0));

  if (address == MAP_FAILED)
  {
    close(fd);
    throw std::system_error(errno, std::system_category());
  }

  // map first region
  auto other_address = static_cast<unsigned char*>(
    mmap(address, capacity, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0));

  if (other_address != address)
  {
    munmap(address, 2 * capacity);
    close(fd);
    throw std::system_error(errno, std::system_category());
  }

  // map second region
  other_address = static_cast<unsigned char*>(
    mmap(address + capacity, capacity, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0));

  if (other_address != address + capacity)
  {
    munmap(address, 2 * capacity);
    close(fd);
    throw std::system_error(errno, std::system_category());
  }

  // we don't need the fd any longer
  if (close(fd) == -1)
  {
    munmap(address, 2 * capacity);
    throw std::system_error(errno, std::system_category());
  }

  // All okay
  return std::pair<unsigned char*, void*>{address, nullptr};
#endif
}

/***/
void destroy_memory_mapped_files(std::pair<unsigned char*, void*> pointer_pair, size_t capacity)
{
  if (!pointer_pair.first)
  {
    return;
  }

#if defined(_WIN32)
  UnmapViewOfFile(pointer_pair.first);
  UnmapViewOfFile(pointer_pair.first + capacity);
  CloseHandle(reinterpret_cast<HANDLE>(pointer_pair.second));
#else
  munmap(pointer_pair.first, 2 * capacity);
#endif
}

} // namespace detail
} // namespace quill