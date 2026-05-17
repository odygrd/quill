/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <windows.h>
#elif defined(__ANDROID__)
// No lock support on Android (no /tmp)
#else
  #include <cerrno>
  #include <cstring>
  #include <fcntl.h>
  #include <sys/file.h>
  #include <unistd.h>
#endif

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"
#include "quill/core/ThreadPrimitives.h"

#include <string>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * @brief Ensures that only one instance of the backend worker is active.
 *
 * This class provides a mechanism to prevent multiple instances of the backend worker
 * from running within the same process. This can occur due to inconsistent linkage
 * (e.g., mixing static and shared libraries), potentially leading to duplicate backend
 * worker threads and unexpected behavior.
 *
 * On Windows, it utilizes a named mutex. On POSIX systems (Linux, macOS), it uses
 * flock() on a lock file in /tmp. The kernel automatically releases the lock when the
 * process exits, even on kill -9, so no stale locks are left behind.
 *
 * Disabled on Android, which lacks /tmp.
 */
class BackendWorkerLock
{
public:
  /***/
  explicit BackendWorkerLock(std::string const& pid)
  {
#if defined(_WIN32)
    std::string name = "Local\\QuillBackendLock" + pid;

    // Create a named mutex. If it already exists in this process, the flag ERROR_ALREADY_EXISTS is set.
    _handle = CreateMutexA(nullptr, TRUE, name.data());

    if (_handle == nullptr)
    {
      QUILL_THROW(QuillError{"Failed to create mutex '" + name + "'"});
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
      // Another instance in the same process already holds the lock.
      CloseHandle(_handle);
      _handle = nullptr;

      QUILL_THROW(QuillError{
        "Duplicate backend worker thread detected. This indicates that the logging library has "
        "been compiled into multiple binary modules (for instance, one module using a static build "
        "and another using a shared build), resulting in separate instances of the backend worker. "
        "Please build and link the logging library uniformly as a shared library with exported "
        "symbols to ensure a single backend instance."});
    }
#elif defined(__ANDROID__)
    // disabled
#else
    std::string path = "/tmp/QuillBackendLock" + pid;

    // Open or create the lock file. The file itself is just a vessel for the kernel lock.
    // Retry on EINTR and transient errors. The lock is a diagnostic aid — if we ultimately
    // cannot open the file, skip the check rather than crashing the application.
    constexpr int max_retries{3};
    constexpr uint64_t retry_delay_ns{100000000}; // 100 ms

    int fd{-1};
    for (int attempt = 0; attempt < max_retries; ++attempt)
    {
      do
      {
        fd = open(path.data(), O_CREAT | O_RDWR, 0644);
      } while (fd == -1 && errno == EINTR);

      if (fd != -1)
      {
        break;
      }

      if (attempt < max_retries - 1)
      {
        sleep_for_ns(retry_delay_ns);
      }
    }

    if (fd == -1)
    {
      return;
    }

    _fd = fd;
    _path = path;

    // Try to acquire an exclusive lock without blocking.
    // Each open() creates a separate file description, so two BackendWorkerLock instances
    // in the same process get independent locks. flock returns -1 with errno EWOULDBLOCK
    // if the lock is already held.
    // Retry on EINTR and transient errors, but EWOULDBLOCK means a genuine duplicate.
    int flock_err{0};
    for (int attempt = 0; attempt < max_retries; ++attempt)
    {
      int ret{0};
      do
      {
        ret = flock(_fd, LOCK_EX | LOCK_NB);
      } while (ret != 0 && errno == EINTR);

      if (ret == 0)
      {
        flock_err = 0;
        break;
      }

      flock_err = errno;

      if (flock_err == EWOULDBLOCK)
      {
        // The lock is genuinely held by another instance — no point retrying.
        break;
      }

      if (attempt < max_retries - 1)
      {
        sleep_for_ns(retry_delay_ns);
      }
    }

    if (flock_err != 0)
    {
      close(_fd);
      _fd = -1;

      if (flock_err == EWOULDBLOCK)
      {
        QUILL_THROW(QuillError{
          "Duplicate backend worker thread detected. This indicates that the logging library has "
          "been compiled into multiple binary modules (for instance, one module using a static "
          "build "
          "and another using a shared build), resulting in separate instances of the backend "
          "worker. "
          "Please build and link the logging library uniformly as a shared library with exported "
          "symbols to ensure a single backend instance."});
      }

      // For any other flock() error (e.g. EIO, ENOMEM, ENOLCK) that persists after
      // retries, the lock is only a diagnostic aid. Skip the check rather than
      // crashing the application.
    }
#endif
  }

  /***/
  ~BackendWorkerLock()
  {
#if defined(_WIN32)
    if (_handle != nullptr)
    {
      ReleaseMutex(_handle);
      CloseHandle(_handle);
      _handle = nullptr;
    }
#elif defined(__ANDROID__)
    // disabled
#else
    if (_fd != -1)
    {
      // Unlink while the lock is still held so the file is removed before anyone else
      // can acquire the same path. On kill -9 the kernel releases the flock but the
      // file remains on disk — harmless, /tmp is cleaned on reboot.
      unlink(_path.data());

      // Closing the fd releases the flock automatically.
      close(_fd);
      _fd = -1;
    }
#endif
  }

  // Disable copy and assignment.
  BackendWorkerLock(BackendWorkerLock const&) = delete;
  BackendWorkerLock& operator=(BackendWorkerLock const&) = delete;

private:
#if defined(_WIN32)
  HANDLE _handle{nullptr};
#elif defined(__ANDROID__)
  // disabled
#else
  int _fd{-1};
  std::string _path;
#endif
};
} // namespace detail

QUILL_END_NAMESPACE
