#include "quill/detail/LogManager.h"

namespace quill::detail
{

/***/
LogManager::LogManager(Config const& config) : _config(config){};

/***/
Logger* LogManager::get_logger(std::string const& logger_name /* = std::string{} */) const
{
  return _logger_collection.get_logger(logger_name);
}

/***/
void LogManager::start_backend_worker() { _backend_worker.run(); }

/***/
void LogManager::stop_backend_worker() { _backend_worker.stop(); }

} // namespace quill::detail