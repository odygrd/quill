/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/BackendOptions.h"
#include "quill/backend/BackendWorker.h"
#include "quill/core/Attributes.h"

#include <cstdint>
#include <mutex>

namespace quill::detail
{
/**
 * Provides access to common collection class that are used by both the frontend and the backend
 * components of the logging system
 * There should only be only active active instance of this class which is achieved by the
 * LogSystemManagerSingleton
 */
class BackendManager
{
public:
  /***/
  static BackendManager& instance() noexcept
  {
    static BackendManager instance;
    return instance;
  }

  /***/
  BackendManager(BackendManager const&) = delete;
  BackendManager& operator=(BackendManager const&) = delete;

  /***/
  QUILL_ATTRIBUTE_COLD void start_backend_thread(BackendOptions const& options)
  {
    // Start the backend worker
    _backend_worker.run(options);
  }

  /***/
  QUILL_ATTRIBUTE_COLD std::once_flag& get_start_once_flag() noexcept { return _start_once_flag; }

  /***/
  QUILL_ATTRIBUTE_COLD void stop_backend_thread() noexcept { _backend_worker.stop(); }

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
  QUILL_NODISCARD uint64_t convert_rdtsc_to_epoch_time(uint64_t rdtsc_value) const noexcept
  {
    return _backend_worker.time_since_epoch(rdtsc_value);
  }

private:
  /***/
  BackendManager() = default;
  ~BackendManager() = default;

private:
  BackendWorker _backend_worker;
  std::once_flag _start_once_flag;
};
} // namespace quill::detail
