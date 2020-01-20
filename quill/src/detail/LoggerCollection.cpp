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

/***/
template <typename TSink, typename... TSinkArgs>
Logger* LoggerCollection::create_logger(std::string const& logger_name, TSinkArgs&&... sink_args)
{
  assert(!logger_name.empty() && "Trying to add a logger with an empty name is not possible");

  std::unique_ptr<SinkBase> sink = std::make_unique<TSink>(std::forward<TSinkArgs>(sink_args)...);

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, std::move(sink), _thread_context_collection)};

  std::scoped_lock lock{_mutex};

  auto [elem_it, inserted] = _logger_name_map.try_emplace(logger_name, std::move(logger));

  assert(inserted && "inserted can not be false");

  // Return the inserted logger
  return elem_it->second.get();
}

} // namespace quill::detail