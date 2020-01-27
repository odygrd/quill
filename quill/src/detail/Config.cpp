#include "quill/detail//Config.h"

namespace quill::detail
{
/***/
void Config::set_backend_thread_sleep_duration(std::chrono::nanoseconds duration) noexcept
{
  _backend_thread_sleep_duration = duration;
}

/***/
std::chrono::nanoseconds Config::backend_thread_sleep_duration() const noexcept
{
  return _backend_thread_sleep_duration;
}

/***/
void Config::set_backend_thread_cpu_affinity(uint16_t cpu) noexcept
{
  _backend_thread_cpu_affinity = cpu;
}

/***/
uint16_t Config::backend_thread_cpu_affinity() const noexcept
{
  return _backend_thread_cpu_affinity;
}

} // namespace quill::detail
