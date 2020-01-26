#include "quill/Quill.h"

#include "quill/detail/LogManagerSingleton.h"

namespace quill
{

/***/
void start_logging_worker()
{
  detail::LogManagerSingleton::instance().log_manager().start_logging_worker();
}

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
} // namespace quill