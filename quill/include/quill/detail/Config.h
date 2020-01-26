#pragma once

#include <chrono>

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
   * Get the backend thread sleep duration
   * @return
   */
  [[nodiscard]] std::chrono::nanoseconds backend_thread_sleep_duration() const noexcept;

private:
  std::chrono::nanoseconds _backend_thread_sleep_duration{500};
};

} // namespace quill::detail