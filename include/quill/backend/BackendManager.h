/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/BackendOptions.h"
#include "quill/backend/BackendWorker.h"
#include "quill/backend/ManualBackendWorker.h"
#include "quill/core/Attributes.h"

#ifndef QUILL_MODULE
  #include <atomic>
  #include <cstdint>
  #include <mutex>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

// Forward declarations
class Backend;
class BackendTscClock;

QUILL_END_EXPORT

namespace detail
{

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4324)
#endif

/**
 * Provides access to common collection class that are used by both the frontend and the backend
 * components of the logging system
 * There should only be only active instance of this class which is achieved by the
 * LogSystemManagerSingleton
 */
class BackendManager
{
public:
  /***/
  QUILL_EXPORT static inline BackendManager& instance() noexcept
  {
    static BackendManager instance;
    return instance;
  }

  /***/
  BackendManager(BackendManager const&) = delete;
  BackendManager& operator=(BackendManager const&) = delete;

private:
  friend class quill::Backend;
  friend class quill::BackendTscClock;

  /***/
  BackendManager() = default;
  ~BackendManager() = default;

  /***/
  QUILL_ATTRIBUTE_COLD void start_backend_thread(BackendOptions const& options)
  {
    // Start the backend worker
    _backend_worker.run(options);
  }

  /**
   * Calls `start` unless the backend was started and has not been stopped since. The backend is
   * not started when `start` throws, and callers that arrive meanwhile wait for it to return.
   */
  template <typename TStart>
  QUILL_ATTRIBUTE_COLD void start_once(TStart&& start)
  {
    std::lock_guard<std::mutex> const start_lock{_start_mutex};

    if (!_is_started)
    {
      start();
      _is_started = true;
    }
  }

  /***/
  QUILL_ATTRIBUTE_COLD void stop_backend_thread() noexcept
  {
    _backend_worker.stop();

    std::lock_guard<std::mutex> const start_lock{_start_mutex};
    _is_started = false;
  }

  /***/
  QUILL_NODISCARD uint32_t get_backend_thread_id() const noexcept
  {
    return _backend_worker.get_backend_thread_id();
  }

  /***/
  void notify_backend_thread() noexcept { _backend_worker.notify(); }

  /***/
  QUILL_NODISCARD bool is_backend_thread_running() const noexcept
  {
    return _backend_worker.is_running();
  }

  /***/
  QUILL_NODISCARD uint64_t convert_rdtsc_to_epoch_time(uint64_t rdtsc_value) const
  {
    return _backend_worker.time_since_epoch(rdtsc_value);
  }

  /***/
  QUILL_NODISCARD ManualBackendWorker* get_manual_backend_worker() noexcept
  {
    return &_manual_backend_worker;
  }

private:
  /***/
  QUILL_NODISCARD bool is_atexit_registered() const noexcept { return _atexit_registered.load(); }

  /***/
  void set_atexit_registered() noexcept { _atexit_registered.store(true); }

private:
  // A std::mutex rather than the Spinlock used elsewhere: stop() holds it while joining the backend
  // thread, through the whole drain and final flush, so a concurrent caller should sleep instead of
  // spinning. Spinlock exists so the frontend headers avoid <mutex>; the backend already uses it.
  std::mutex _stop_mutex;
  std::mutex _start_mutex;
  BackendWorker _backend_worker;
  ManualBackendWorker _manual_backend_worker{&_backend_worker};
  std::atomic<bool> _atexit_registered{false};
  bool _is_started{false}; /**< Protected by _start_mutex */
};

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif

} // namespace detail

QUILL_END_NAMESPACE
