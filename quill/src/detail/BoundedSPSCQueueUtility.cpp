#include "quill/detail/BoundedSPSCQueueUtility.h"

#include <cstdint>

#include "quill/detail/CommonUtilities.h"
#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <windows.h>
#else
  #include <sys/mman.h>
  #include <unistd.h>
#endif

namespace quill
{
namespace detail
{

/***/
std::pair<unsigned char*, void*> create_memory_mapped_files(size_t capacity)
{
#if defined(_WIN32)
  HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                      static_cast<DWORD>(capacity * 2), NULL);

  if (!hMapFile)
  {
    // TODO:: GetLastError message
    throw std::runtime_error("Could not create file mapping");
  }

  for (;;)
  {
    // find a free address space with the correct size
    auto address =
      static_cast<unsigned char*>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, capacity * 2));

    if (!address)
    {
      CloseHandle(hMapFile);
      // TODO:: GetLastError message
      throw std::runtime_error("Could not create file mapping");
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

        // TODO:: GetLastError message
        throw std::runtime_error("Could not create file mapping");
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

        // TODO:: GetLastError message
        throw std::runtime_error("Could not create file mapping");
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
      std::ostringstream error_message;
      error_message << "Could not open file in any location. Error : " << std::strerror(errno);
      throw std::runtime_error(error_message.str());
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
    close(fd);
    std::ostringstream error_message;
    error_message << "Could not unlink file. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  if (ftruncate(fd, static_cast<off_t>(capacity())) == -1)
  {
    close(fd);
    std::ostringstream error_message;
    error_message << "Could not truncate file. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // ask mmap for a good address where we can put both virtual copies of the buffer
  auto address = static_cast<unsigned char*>(mmap(nullptr, 2 * capacity(), PROT_NONE, MMAP_FLAGS, -1, 0));

  if (address == MAP_FAILED)
  {
    close(fd);
    std::ostringstream error_message;
    error_message << "Failed to mmap. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // map first region
  auto other_address = static_cast<unsigned char*>(
    mmap(address, capacity(), PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, fd, 0));
  if (other_address != address)
  {
    munmap(address, 2 * capacity());
    close(fd);
    std::ostringstream error_message;
    error_message << "Failed to mmap. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // map second region
  other_address = static_cast<unsigned char*>(
    mmap(address + capacity(), capacity(), PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE, fd, 0));
  if (other_address != address + capacity())
  {
    munmap(address, 2 * capacity());
    close(fd);
    std::ostringstream error_message;
    error_message << "Failed to mmap. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // we don't need the fd any longer
  if (close(fd) == -1)
  {
    munmap(address, 2 * capacity());
    std::ostringstream error_message;
    error_message << "Failed to close the fd. Error : " << std::strerror(errno);
    throw std::runtime_error(error_message.str());
  }

  // All okay
  return std::pair < std::pair<unsigned char*, void*>{address, nullptr};

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

/***/
size_t page_size() noexcept 
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
} // namespace detail
} // namespace quill