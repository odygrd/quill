#include "quill/detail/LoggerCollection.h"

#include <cassert>
#include <sstream>
#include <stdexcept>

#include "quill/detail/Macros.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/Utilities.h"
#include "quill/sinks//StdoutSink.h"

namespace quill::detail
{
static constexpr char const* _default_logger_name{"root"};

/***/
LoggerCollection::LoggerCollection(ThreadContextCollection& thread_context_collection)
  : _thread_context_collection(thread_context_collection)
{
  _logger_name_map.reserve(4);

  // Add the default logger to Stdout sink
  _default_logger = create_logger<StdoutSink>(_default_logger_name);
}

/***/
Logger* LoggerCollection::get_logger(std::string const& logger_name /* = std::string{} */) const
{
  if (!logger_name.empty())
  {
    std::scoped_lock lock{_mutex};

    // Search for the logger
    auto const search = _logger_name_map.find(logger_name);
    if (QUILL_UNLIKELY(search == _logger_name_map.cend()))
    {
      // logger does not exist
      throw std::runtime_error("logger with name " + logger_name + " does not exist.");
    }

    return search->second.get();
  }
  else
  {
    return _default_logger;
  }
}

} // namespace quill::detail