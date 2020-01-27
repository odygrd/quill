#pragma once

#include <chrono>
#include <cstdint>
#include <limits>

namespace quill::detail
{
class Config
{
public:
  /**
   * Sets the backend thread sleep duration
   * @param duration
   */
  void set_backend_thread_sleep_duration(std::chrono::nanoseconds duration) noexcept;

  /**
   * @return The backend thread sleep duration when idle
   */
  [[nodiscard]] std::chrono::nanoseconds backend_thread_sleep_duration() const noexcept;

  /**
   * Pins the backend thread to the given cpu
   * @param cpu
   */
  void set_backend_thread_cpu_affinity(uint16_t cpu) noexcept;

  /**
   * @return The backend thread cpu affinity
   */
  [[nodiscard]] uint16_t backend_thread_cpu_affinity() const noexcept;

private:
  std::chrono::nanoseconds _backend_thread_sleep_duration{500};
  uint16_t _backend_thread_cpu_affinity{
    std::numeric_limits<uint16_t>::max()}; /** max() as undefined value, cpu affinity will not be set */
};

} // namespace quill::detail