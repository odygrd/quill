#include "quill/detail/LogManager.h"

namespace quill::detail
{

/***/
LogManager::LogManager(Config const& config) : _config(config){};

/***/
void LogManager::start_backend_worker() { _backend_worker.run(); }

/***/
void LogManager::stop_backend_worker() { _backend_worker.stop(); }

} // namespace quill::detail