#include "quill/detail/LogManager.h"

namespace quill::detail
{
/***/
Logger* LogManager::get_logger(std::string const& logger_name /* = std::string{} */) const
{
  return _logger_collection.get_logger(logger_name);
}
} // namespace quill::detail