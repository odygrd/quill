/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

namespace quill
{
namespace detail
{
class Config
{
public:
  /**
   * Sets the backend thread sleep duration
   * @param duration sleep duration in nanoseconds
   */
  QUILL_ATTRIBUTE_COLD void set_backend_thread_sleep_duration(std::chrono::nanoseconds duration) noexcept;

  /**
   * @return The backend thread sleep duration when idle
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::chrono::nanoseconds backend_thread_sleep_duration() const noexcept;

  /**
   * Pins the backend thread to the given cpu
   * @param cpu desired cpu to pin the thread
   */
  QUILL_ATTRIBUTE_COLD void set_backend_thread_cpu_affinity(uint16_t cpu) noexcept;

  /**
   * @return The backend thread cpu affinity
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD uint16_t backend_thread_cpu_affinity() const noexcept;

  /**
   * Names the backend thread
   * @param name the desired name
   */
  QUILL_ATTRIBUTE_COLD void set_backend_thread_name(std::string const& name) noexcept;

  /**
   * @return The backend thread name
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::string backend_thread_name() const noexcept;

  /**
   * Sets the initial queue capacity
   * @param initial_capacity initial queue capacity
   */
  QUILL_ATTRIBUTE_COLD void set_initial_queue_capacity(std::size_t initial_capacity) noexcept;

  /**
   * @return get the initial queue capacity
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD size_t initial_queue_capacity() const noexcept;

private:
  std::string _backend_thread_name{"Quill_Backend"}; /** Custom name for the backend thread */
  std::chrono::nanoseconds _backend_thread_sleep_duration{300};

#if defined(_WIN32)
  size_t _initial_queue_capacity{65536}; /**< Initial queue capacity default to 64K due to allocation granularity */
#else
  size_t _initial_queue_capacity{16384}; /**< Initial queue capacity default to 16K */
#endif

  uint16_t _backend_thread_cpu_affinity{
    (std::numeric_limits<uint16_t>::max)()}; /** max() as undefined value, cpu affinity will not be set */
};
} // namespace detail
} // namespace quill