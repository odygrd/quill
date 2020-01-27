#include "quill/Quill.h"

#include "quill/detail/LogManagerSingleton.h"

namespace quill
{

/***/
void start() { detail::LogManagerSingleton::instance().log_manager().start_backend_worker(); }

/***/
Logger* get_logger(std::string const& logger_name /* = std::string{} */)
{
  return detail::LogManagerSingleton::instance().log_manager().get_logger(logger_name);
}

/***/
Logger* create_logger(std::string logger_name)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(std::move(logger_name));
}

/***/
Logger* create_logger(std::string logger_name, std::unique_ptr<SinkBase> sink)
{
  return detail::LogManagerSingleton::instance().log_manager().create_logger(std::move(logger_name),
                                                                             std::move(sink));
}

// ** Config ** //
namespace config
{

/***/
void set_backend_thread_sleep_duration(std::chrono::nanoseconds sleep_duration) noexcept
{
  detail::LogManagerSingleton::instance().config().set_backend_thread_sleep_duration(sleep_duration);
}

/***/
void set_backend_thread_cpu_affinity(uint16_t cpu) noexcept
{
  detail::LogManagerSingleton::instance().config().set_backend_thread_cpu_affinity(cpu);
}
} // namespace config

} // namespace quill