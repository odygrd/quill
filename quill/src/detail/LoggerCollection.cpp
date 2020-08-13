#include "quill/detail/LoggerCollection.h"
#include "quill/QuillError.h"               // for QUILL_THROW, QuillError
#include "quill/detail/HandlerCollection.h" // for HandlerCollection
#include "quill/detail/LoggerDetails.h"     // for LoggerDetails
#include "quill/detail/misc/Macros.h"       // for QUILL_UNLIKELY
#include "quill/handlers/StreamHandler.h"   // for StreamHandler
#include <mutex>                            // for lock_guard
#include <utility>                          // for move, pair
#include <vector>                           // for vector

namespace quill
{
namespace detail
{
static constexpr char const* _default_logger_name{"root"};

/***/
LoggerCollection::LoggerCollection(ThreadContextCollection& thread_context_collection,
                                   HandlerCollection& handler_collection)
  : _thread_context_collection(thread_context_collection), _handler_collection(handler_collection)
{
  // Pre-allocate early to a reasonable size
  _logger_name_map.reserve(16);

  // Add the default console handler to the default logger
  Handler* stdout_stream_handler = _handler_collection.stdout_console_handler("stdout");
  _default_logger = create_logger(_default_logger_name, stdout_stream_handler);
}

/***/
Logger* LoggerCollection::get_logger(char const* logger_name /* = nullptr */) const
{
  if (logger_name)
  {
    std::string logger_name_str{logger_name};
    std::lock_guard<RecursiveSpinlock> const lock{_spinlock};

    // Search for the logger
    auto const search = _logger_name_map.find(logger_name_str);
    if (QUILL_UNLIKELY(search == _logger_name_map.cend()))
    {
      // logger does not exist
      QUILL_THROW(QuillError{std::string{"logger does not exist. name: "} + logger_name_str});
    }

    return (*search).second.get();
  }
  else
  {
    return _default_logger;
  }
}

/***/
Logger* LoggerCollection::create_logger(char const* logger_name)
{
  // Get a copy of the default logger handlers
  std::vector<Handler*> handlers = _default_logger->_logger_details.handlers();

  // Register the handlers, even if they already exist
  for (auto& handler : handlers)
  {
    _handler_collection.subscribe_handler(handler);
  }

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, std::move(handlers), _thread_context_collection)};

  std::lock_guard<RecursiveSpinlock> const lock{_spinlock};

  // Place the logger in our map
  auto const insert_result = _logger_name_map.emplace(std::string{logger_name}, std::move(logger));

  // Return the inserted logger or the existing logger
  return (*insert_result.first).second.get();
}

/***/
Logger* LoggerCollection::create_logger(char const* logger_name, Handler* handler)
{
  // Register the handler, even if it already exist
  _handler_collection.subscribe_handler(handler);

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, handler, _thread_context_collection)};

  std::lock_guard<RecursiveSpinlock> const lock{_spinlock};

  auto const insert_result = _logger_name_map.emplace(std::string{logger_name}, std::move(logger));

  // Return the inserted logger or the existing logger
  return (*insert_result.first).second.get();
}

/***/
Logger* LoggerCollection::create_logger(char const* logger_name, std::initializer_list<Handler*> handlers)
{
  // Register the handlers, even if they already exist
  for (auto& handler : handlers)
  {
    _handler_collection.subscribe_handler(handler);
  }

  // We can't use make_unique since the constructor is private
  std::unique_ptr<Logger> logger{new Logger(logger_name, handlers, _thread_context_collection)};

  std::lock_guard<RecursiveSpinlock> const lock{_spinlock};

  auto const insert_result = _logger_name_map.emplace(std::string{logger_name}, std::move(logger));

  // Return the inserted logger or the existing logger
  return (*insert_result.first).second.get();
}

/***/
void LoggerCollection::set_default_logger_handler(Handler* handler)
{
  std::lock_guard<RecursiveSpinlock> const lock{_spinlock};

  // Remove the old default logger
  _logger_name_map.erase(std::string{_default_logger_name});

  // Remake the default logger
  _default_logger = create_logger(_default_logger_name, handler);
}

/***/
void LoggerCollection::set_default_logger_handler(std::initializer_list<Handler*> handlers)
{
  std::lock_guard<RecursiveSpinlock> const lock{_spinlock};

  // Remove the old default logger
  _logger_name_map.erase(std::string{_default_logger_name});

  // Remake the default logger
  _default_logger = create_logger(_default_logger_name, std::move(handlers));
}

/***/
void LoggerCollection::enable_console_colours() noexcept
{
  // Get the previous created default stdout handler
  Handler* stdout_stream_handler = _handler_collection.stdout_console_handler("stdout");
  assert(stdout_stream_handler && "stdout_stream_handler can not be nullptr");

  auto console_handler = reinterpret_cast<ConsoleHandler*>(stdout_stream_handler);
  console_handler->enable_console_colours();
}
} // namespace detail
} // namespace quill