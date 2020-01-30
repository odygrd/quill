#include "quill/detail/LoggerCollection.h"

#include <cassert>
#include <sstream>
#include <stdexcept>

#include "quill/detail/CommonMacros.h"
#include "quill/detail/CommonUtilities.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/handlers/StreamHandler.h"

namespace quill::detail
{
static constexpr char const* _default_logger_name{"root"};

/***/
LoggerCollection::LoggerCollection(ThreadContextCollection& thread_context_collection,
                                   HandlerCollection& handler_collection)
  : _thread_context_collection(thread_context_collection), _handler_collection(handler_collection)
{
  // Pre-allocate early to a reasonable size
  _logger_name_map.reserve(16);
  _logger_cache.reserve(16);

  // Add the default std streamhandler to the default logger
  Handler* stdout_stream_handler = _handler_collection.stdout_streamhandler();
  _default_logger = create_logger(_default_logger_name, stdout_stream_handler);
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
  // Get a copy of the default logger handlers
  std::vector<Handler*> handlers = _default_logger->_logger_details.handlers();

  // Register the handlers, even if they already exist
  for (auto& handler : handlers)
  {
    // TODO: could improve the lock
    _handler_collection.subscribe_handler(handler);
  }

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, std::move(handlers), _thread_context_collection)};

  std::scoped_lock lock{_mutex};

  // Place the logger in our map
  auto [elem_it, inserted] = _logger_name_map.try_emplace(std::move(logger_name), std::move(logger));

  // Return the inserted logger
  return elem_it->second.get();
}

/***/
Logger* LoggerCollection::create_logger(std::string logger_name, Handler* handler)
{
  assert(!logger_name.empty() && "Trying to add a logger with an empty name is not possible");

  // Register the handler, even if it already exist
  _handler_collection.subscribe_handler(handler);

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, handler, _thread_context_collection)};

  std::scoped_lock lock{_mutex};

  auto [elem_it, inserted] = _logger_name_map.try_emplace(std::move(logger_name), std::move(logger));

  assert(inserted && "inserted can not be false");

  // Return the inserted logger
  return elem_it->second.get();
}

/***/
Logger* LoggerCollection::create_logger(std::string logger_name, std::initializer_list<Handler*> handlers)
{
  assert(!logger_name.empty() && "Trying to add a logger with an empty name is not possible");

  // Register the handlers, even if they already exist
  for (auto& handler : handlers)
  {
    // TODO: could improve the lock
    _handler_collection.subscribe_handler(handler);
  }

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, handlers, _thread_context_collection)};

  std::scoped_lock lock{_mutex};

  auto [elem_it, inserted] = _logger_name_map.try_emplace(std::move(logger_name), std::move(logger));

  assert(inserted && "inserted can not be false");

  // Return the inserted logger
  return elem_it->second.get();
}

/***/
void LoggerCollection::set_default_logger_handler(Handler* handler)
{
  std::scoped_lock lock{_mutex};

  // Remove the old default logger
  _logger_name_map.erase(_default_logger_name);

  // Remake the default logger
  _default_logger = create_logger(_default_logger_name, handler);
}

/***/
void LoggerCollection::set_default_logger_handler(std::initializer_list<Handler*> handlers)
{
  std::scoped_lock lock{_mutex};

  // Remove the old default logger
  _logger_name_map.erase(_default_logger_name);

  // Remake the default logger
  _default_logger = create_logger(_default_logger_name, std::move(handlers));
}

} // namespace quill::detail