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
} // namespace quill::detail

// #define QUILL_BACKEND_THREAD_SLEEP_DURATION_NS 500u
