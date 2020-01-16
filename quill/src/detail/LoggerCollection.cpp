#include "quill/detail/LoggerCollection.h"

#include <cassert>
#include <sstream>
#include <stdexcept>

#include "quill/detail/Macros.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/Utilities.h"

namespace quill::detail
{

static constexpr char const* _default_logger_name{"root"};

/***/
LoggerCollection::LoggerCollection(ThreadContextCollection& thread_context_collection)
  : _thread_context_collection(thread_context_collection)
{
  _logger_name_map.reserve(4);
  _logger_id_map.reserve(4);

  // Add the default logger
  _default_logger = _create_logger(_default_logger_name);
}

/***/
Logger* LoggerCollection::get_logger(std::string const& logger_name /* = std::string{} */) const
{
  return logger_name.empty() ? _default_logger : _find_or_create_logger(logger_name);
}

/***/
char const* LoggerCollection::get_logger_name(uint16_t logger_id) const
{
  // first check against the default logger
  if (logger_id == _default_logger->id())
  {
    return _default_logger_name;
  }

  std::scoped_lock lock{_mutex};

  // Search for the logger id
  auto const search = _logger_id_map.find(logger_id);

  if (QUILL_UNLIKELY(search == _logger_id_map.cend()))
  {
    std::ostringstream error_message;
    error_message << "Could not find existing logger for logger id " << logger_id;
    throw std::runtime_error(error_message.str());
  }

  return search->second.data();
}

/***/
Logger* LoggerCollection::_create_logger(std::string const& logger_name) const
{
  assert(!logger_name.empty() && "Trying to add a logger with an empty name is not possible");

  std::scoped_lock lock{_mutex};

  // Add the logger with a new id
  _logger_id += 1;

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(_thread_context_collection, _logger_id)};
  auto [elem_it, inserted] = _logger_name_map.try_emplace(logger_name, std::move(logger));

  assert(inserted && "inserted can not be false");

  // Also add the mapping from id to name
  _logger_id_map[_logger_id] = logger_name;

  // Return the inserted logger
  return elem_it->second.get();
}

/***/
Logger* LoggerCollection::_find_or_create_logger(std::string const& logger_name) const
{
  assert(!logger_name.empty() && "Trying to find a logger with an empty name is not possible");

  std::scoped_lock lock{_mutex};

  // Search for the logger
  auto const search = _logger_name_map.find(logger_name);
  if (search != _logger_name_map.cend())
  {
    return search->second.get();
  }

  return _create_logger(logger_name);
}
} // namespace quill::detail