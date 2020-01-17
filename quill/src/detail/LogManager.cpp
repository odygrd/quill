#include "quill/detail/LogManager.h"

namespace quill::detail
{
/***/
Logger* LogManager::get_logger(std::string const& logger_name /* = std::string{} */) const
{
  return _logger_collection.get_logger(logger_name);
}

/***/
void LogManager::start_logging_worker() { _logging_worker.run(); }

/***/
void LogManager::stop_logging_worker() { _logging_worker.stop(); }

} // namespace quill::detail