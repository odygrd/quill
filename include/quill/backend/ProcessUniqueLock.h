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
#else
#include <semaphore.h>
#include <fcntl.h>      // For O_CREAT and O_EXCL
#endif

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"

#include <string>

QUILL_BEGIN_NAMESPACE

/**
 * Due to Windows linkage issues when mixing shared libraries and static libraries, it is possible
 * for the singleton that initializes the backend to be instantiated more than once.
 * This situation may lead to multiple backend worker threads running simultaneously, which can
 * cause unexpected behavior and crashes.
 *
 * Specifically, when the singleton is compiled into a static library that is linked into both a shared
 * library and the main executable, separate instances may be created. The recommended solution is to
 * compile the singleton into a shared library and export its symbols (e.g., using WINDOWS_EXPORT_ALL_SYMBOLS).
 *
 * This function retrieves all thread names in the current process and checks if one of them matches
 * the backend worker thread name specified in _options.thread_name. If a match is found, a QuillError
 * is thrown with a detailed explanation.
 *
 * @throw QuillError if a duplicate backend worker thread instance is detected.
 */
class ProcessUniqueLock
{
public:
  explicit ProcessUniqueLock(std::string const& pid)
  {
#if defined(_WIN32)
    std::string name = "Local\\QuillLock_" + pid;

    // Create a named mutex. If it already exists in this process, the flag ERROR_ALREADY_EXISTS is set.
    _handle = CreateMutexA(nullptr, TRUE, name.data());

    if (_handle == nullptr)
    {
      QUILL_THROW(QuillError{"Failed to create mutex"});
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
      // Another instance in the same process already holds the lock.
      QUILL_THROW(QuillError{
        "Duplicate backend worker thread detected. This indicates that the logging library has "
        "been compiled into multiple binary modules (for instance, one module using a static build "
        "and another using a shared build), resulting in separate instances of the backend worker. "
        "Please build and link the logging library uniformly as a shared library with exported "
        "symbols to ensure a single backend instance."});
    }
#else
    std::string name = "QuillLock_" + pid;

    // Open or create the named semaphore.
    // O_CREAT will create the semaphore if it doesn't exist, and if it does exist it will simply open the same semaphore.
    _sem = sem_open(name.data(), O_CREAT, 0644, 1);
    if (_sem == SEM_FAILED) {
      QUILL_THROW(QuillError{"Failed to create semaphore"});
    }

    // Immediately unlink it so that it leaves no traces.
    // The semaphore will still exist until all descriptors are closed.
    sem_unlink(name.data());

    // Try to lock the semaphore.
    // If it’s already locked (by another instance within the same process),
    // sem_trywait will return -1 and set errno.
    if (sem_trywait(_sem) != 0) {
      QUILL_THROW(QuillError{
        "Duplicate backend worker thread detected. This indicates that the logging library has "
        "been compiled into multiple binary modules (for instance, one module using a static build "
        "and another using a shared build), resulting in separate instances of the backend worker. "
        "Please build and link the logging library uniformly as a shared library with exported "
        "symbols to ensure a single backend instance."});
    }
#endif
  }

  ~ProcessUniqueLock()
  {
#if defined(_WIN32)
    if (_handle != nullptr)
    {
      ReleaseMutex(_handle);
      CloseHandle(_handle);
      _handle = nullptr;
    }
#else
    if (_sem != SEM_FAILED) {
      sem_post(_sem);
      sem_close(_sem);
      _sem = SEM_FAILED;
    }
#endif
  }

  // Disable copy and assignment.
  ProcessUniqueLock(const ProcessUniqueLock&) = delete;
  ProcessUniqueLock& operator=(const ProcessUniqueLock&) = delete;

private:
#if defined(_WIN32)
  HANDLE _handle{nullptr};
#else
  sem_t* _sem{SEM_FAILED};
#endif
};

QUILL_END_NAMESPACE