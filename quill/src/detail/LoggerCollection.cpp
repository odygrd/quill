#include "quill/detail/LoggerCollection.h"

#include <cassert>
#include <sstream>
#include <stdexcept>

#include "quill/detail/CommonMacros.h"
#include "quill/detail/CommonUtilities.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/sinks//StdoutSink.h"

namespace quill::detail
{
static constexpr char const* _default_logger_name{"root"};

/***/
LoggerCollection::LoggerCollection(ThreadContextCollection& thread_context_collection)
  : _thread_context_collection(thread_context_collection)
{
  _logger_name_map.reserve(4);

  // Add the default StdoutSink to the default logger
  std::unique_ptr<SinkBase> stdout_sink = std::make_unique<StdoutSink>();
  _default_logger = create_logger(_default_logger_name, std::move(stdout_sink));
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
Logger* LoggerCollection::create_logger(std::string logger_name)
{
  // default logger is always using only one sink
  std::unique_ptr<SinkBase> sink{_default_logger->_logger_details.sinks()[0]->clone()};

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, std::move(sink), _thread_context_collection)};

  std::scoped_lock lock{_mutex};

  auto [elem_it, inserted] = _logger_name_map.try_emplace(std::move(logger_name), std::move(logger));

  assert(inserted && "inserted can not be false");

  // Return the inserted logger
  return elem_it->second.get();
}

/***/
Logger* LoggerCollection::create_logger(std::string logger_name, std::unique_ptr<SinkBase> sink)
{
  assert(!logger_name.empty() && "Trying to add a logger with an empty name is not possible");

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, std::move(sink), _thread_context_collection)};

  std::scoped_lock lock{_mutex};

  auto [elem_it, inserted] = _logger_name_map.try_emplace(std::move(logger_name), std::move(logger));

  assert(inserted && "inserted can not be false");

  // Return the inserted logger
  return elem_it->second.get();
}

/***/
void LoggerCollection::set_custom_default_logger(std::unique_ptr<SinkBase> sink)
{
  std::scoped_lock lock{_mutex};

  // Remove the old default logger
  _logger_name_map.erase(_default_logger_name);

  // Remake the default logger
  _default_logger = create_logger(_default_logger_name, std::move(sink));
}

/***/
void LoggerCollection::_set_custom_default_logger(std::vector<std::unique_ptr<SinkBase>> sink_collection)
{
  std::scoped_lock lock{_mutex};

  // Remove the old default logger
  _logger_name_map.erase(_default_logger_name);

  // Remake the default logger
  _default_logger = _create_logger(_default_logger_name, std::move(sink_collection));
}

/***/
Logger* LoggerCollection::_create_logger(std::string logger_name, std::vector<std::unique_ptr<SinkBase>> sinks_collection)
{
  assert(!logger_name.empty() && "Trying to add a logger with an empty name is not possible");

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, std::move(sinks_collection), _thread_context_collection)};

  std::scoped_lock lock{_mutex};

  auto [elem_it, inserted] = _logger_name_map.try_emplace(std::move(logger_name), std::move(logger));

  assert(inserted && "inserted can not be false");

  // Return the inserted logger
  return elem_it->second.get();
}
} // namespace quill::detail