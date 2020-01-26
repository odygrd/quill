#include "quill/detail/LogManagerSingleton.h"

namespace quill::detail
{

/***/
LogManagerSingleton::~LogManagerSingleton()
{
  // always call stop on destruction to log everything
  _log_manager.stop_logging_worker();
}

/***/
LogManagerSingleton& LogManagerSingleton::instance() noexcept
{
  static LogManagerSingleton instance;
  return instance;
}

/***/
detail::LogManager& LogManagerSingleton::log_manager() noexcept { return _log_manager; }
} // namespace quill::detail