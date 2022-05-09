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
  QUILL_ATTRIBUTE_COLD void set_backend_thread_sleep_duration(std::chrono::nanoseconds duration) noexcept
  {
    _backend_thread_sleep_duration = duration;
  }

  /**
   * @return The backend thread sleep duration when idle
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::chrono::nanoseconds backend_thread_sleep_duration() const noexcept
  {
    return _backend_thread_sleep_duration;
  }

  /**
   * Pins the backend thread to the given cpu
   * @param cpu desired cpu to pin the thread
   */
  QUILL_ATTRIBUTE_COLD void set_backend_thread_cpu_affinity(uint16_t cpu) noexcept
  {
    _backend_thread_cpu_affinity = cpu;
  }

  /**
   * @return The backend thread cpu affinity
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD uint16_t backend_thread_cpu_affinity() const noexcept
  {
    return _backend_thread_cpu_affinity;
  }

  /**
   * Names the backend thread max_transit_events
   * @param max_transit_events the max number before we flush the queue
   */
  QUILL_ATTRIBUTE_COLD void set_backend_thread_max_transit_events(size_t max_transit_events) noexcept
  {
    _backend_thread_max_transit_events = max_transit_events;
  }

  /**
   * @return get the backend thread max_transit_events
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD size_t backend_thread_max_transit_events() const noexcept
  {
    return _backend_thread_max_transit_events;
  }

  /**
   * Names the backend thread
   * @param name the desired name
   */
  QUILL_ATTRIBUTE_COLD void set_backend_thread_name(std::string const& name) noexcept
  {
    _backend_thread_name = name;
  }

  /**
   * @return The backend thread name
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::string backend_thread_name() const noexcept
  {
    return _backend_thread_name;
  }

private:
  std::string _backend_thread_name{"Quill_Backend"}; /** Custom name for the backend thread */
  std::chrono::nanoseconds _backend_thread_sleep_duration{300};
  size_t _backend_thread_max_transit_events{800};
  uint16_t _backend_thread_cpu_affinity{
    (std::numeric_limits<uint16_t>::max)()}; /** max() as undefined value, cpu affinity will not be set */
};
} // namespace detail
} // namespace quill