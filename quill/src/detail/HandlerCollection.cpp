#include "quill/detail/HandlerCollection.h"
#include "quill/QuillError.h"              // for QuillError
#include "quill/detail/misc/Utilities.h"   // for s2ws
#include "quill/handlers/ConsoleHandler.h" // for ConsoleHandler
#include <algorithm>                       // for find_if
#include <cstdio>                          // for stdout,stderr
#include <utility>                         // for pair

namespace quill::detail
{
/***/
std::shared_ptr<Handler> HandlerCollection::stdout_console_handler(std::string const& stdout_handler_name /* = std::string{"stdout"} */,
                                                                   ConsoleColours const& console_colours /* = ConsoleColours{} */)
{
  return _create_console_handler(stdout_handler_name, stdout, console_colours);
}

/***/
std::shared_ptr<Handler> HandlerCollection::stderr_console_handler(std::string const& stderr_handler_name /* = std::string{"stderr"} */)
{
  // we just pass an empty ConsoleColours class, as we don't use colours for stderr.
  return _create_console_handler(stderr_handler_name, stderr, quill::ConsoleColours{});
}

/***/
std::shared_ptr<Handler> HandlerCollection::get_handler(std::string const& handler_name)
{
  // Protect shared access
  std::lock_guard<std::mutex> const lock{_mutex};

  // Try to insert it unless we failed it means we already had it
  auto const search = _handler_collection.find(handler_name);

  if (search == _handler_collection.cend())
  {
    QUILL_THROW(QuillError{"Handler with name " + handler_name + " does not exist"});
  }

  std::shared_ptr<Handler> handler = search->second.lock();
  return handler;
}

/***/
void HandlerCollection::subscribe_handler(std::shared_ptr<Handler> const& handler_to_insert)
{
  // Protect shared access
  std::lock_guard<std::mutex> const lock{_mutex};

  // Check if we already have this object
  auto const search = std::find_if(_active_handlers_collection.cbegin(), _active_handlers_collection.cend(),
                                   [&handler_to_insert](std::weak_ptr<Handler> const& handler_elem)
                                   {
                                     std::shared_ptr<Handler> const elem = handler_elem.lock();
                                     if (!elem)
                                     {
                                       return false;
                                     }
                                     else
                                     {
                                       return elem.get() == handler_to_insert.get();
                                     }
                                   });

  if (search == _active_handlers_collection.cend())
  {
    // we don't have this object so add it
    _active_handlers_collection.push_back(handler_to_insert);
  }
}

/***/
std::vector<std::weak_ptr<Handler>> HandlerCollection::active_handlers() const
{
  std::vector<std::weak_ptr<Handler>> subscribed_handlers_collection;

  // Protect shared access, we just use a lock here since this function is not used when logging
  // messages but only in special cases e.g. flushing
  std::lock_guard<std::mutex> const lock{_mutex};
  subscribed_handlers_collection = _active_handlers_collection;

  return subscribed_handlers_collection;
}

/***/
void HandlerCollection::remove_unused_handlers()
{
  // this needs to take a lock each time. The backend logging thread should be carefully call
  // it only when needed
  std::lock_guard<std::mutex> const lock{_mutex};
  for (auto it = std::begin(_handler_collection); it != std::end(_handler_collection);)
  {
    if (it->second.expired())
    {
      it = _handler_collection.erase(it);
    }
    else
    {
      ++it;
    }
  }

  _active_handlers_collection.erase(
    std::remove_if(std::begin(_active_handlers_collection), std::end(_active_handlers_collection),
                   [](std::weak_ptr<Handler> const& elem) { return elem.expired(); }),
    std::end(_active_handlers_collection));
}

/***/
std::shared_ptr<Handler> HandlerCollection::_create_console_handler(std::string const& stream, FILE* file,
                                                                    ConsoleColours const& console_colours)
{
  // Protect shared access
  std::lock_guard<std::mutex> const lock{_mutex};

  // Try to insert it unless we failed it means we already had it
  auto const search = _handler_collection.find(stream);

  // First search if we have it and don't create it yet as this will call fopen
  if (search != _handler_collection.cend())
  {
    // since we found a similar handler we assume that it derives from StreamHandler
    auto handler_ptr = search->second.lock();

    if (handler_ptr)
    {
      auto handler = reinterpret_cast<StreamHandler*>(handler_ptr.get());

      // if the handler with that name was already created check that the user didn't try to create it again passing a different file
      if (file == stdout && (handler->stream_handler_type() != StreamHandler::StreamHandlerType::Stdout))
      {
        QUILL_THROW(QuillError(
          "Trying to insert an stdout handler again, but the handler already exists as a different "
          "file. Use an unique stream_handler name"));
      }
      else if (file == stderr && (handler->stream_handler_type() != StreamHandler::StreamHandlerType::Stderr))
      {
        QUILL_THROW(QuillError(
          "Trying to insert an stderr handler again, but the handler already exists as a different "
          "file. Use an unique stream_handler name"));
      }
      else if (handler->stream_handler_type() == StreamHandler::StreamHandlerType::File)
      {
        QUILL_THROW(
          QuillError("Trying to insert an stdout/stderr handler, but the handler already exists. "
                     "Use an unique "
                     "stream_handler name"));
      }

      return handler_ptr;
    }
    else
    {
      // found a similar handler but with an out of date weak ptr, this is very rare to happen
      // as the backend logging thread is cleaning them
      // in that case allocate a new handler in that location
      handler_ptr = std::make_shared<ConsoleHandler>(stream, file, console_colours);
      search->second = handler_ptr;
      return handler_ptr;
    }
  }

  // if first time add it
  auto handler_ptr = std::make_shared<ConsoleHandler>(stream, file, console_colours);
  _handler_collection.emplace(stream, handler_ptr);
  return handler_ptr;
}
} // namespace quill::detail