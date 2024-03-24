/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/common/Attributes.h"       // for QUILL_NODISCARD, QUILL_ATT...
#include "quill/common/QuillError.h"       // for QuillError
#include "quill/common/Utilities.h"        // for s2ws
#include "quill/handlers/ConsoleHandler.h" // for ConsoleColours
#include "quill/handlers/FileHandler.h"    // for FilenameAppend
#include "quill/handlers/StreamHandler.h"  // for StreamHandler
#include <algorithm>                       // for find_if
#include <chrono>                          // for hours, minutes
#include <cstddef>                         // for size_t
#include <cstdio>                          // for stdout,stderr
#include <memory>                          // for allocator, unique_ptr
#include <memory>
#include <mutex>  // for lock_guard
#include <string> // for string, hash
#include <type_traits>
#include <unordered_map> // for unordered_map
#include <utility>       // for pair
#include <vector>        // for vector

namespace quill
{

/** forward declarations **/
class Handler;

namespace detail
{
/**
 * Creates and manages active handlers
 */
class HandlerCollection
{
public:
  HandlerCollection() = default;
  ~HandlerCollection() = default;

  /**
   * Deleted
   */
  HandlerCollection(HandlerCollection const&) = delete;
  HandlerCollection& operator=(HandlerCollection const&) = delete;

  /**
   * The handlers are used by the backend thread, so after their creation we want to avoid
   * mutating their member variables. So here the API returns pointers to the base class
   * to somehow restrict the user from creating a handler and calling a `set()` function
   * on the handler after it's creation.
   * Currently no built-in handlers have setters function.
   */

  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> stdout_console_handler(
    std::string const& stdout_handler_name = std::string{"stdout"},
    ConsoleColours const& console_colours = ConsoleColours{})
  {
    return _create_console_handler(stdout_handler_name, stdout, console_colours);
  }

  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> stderr_console_handler(
    std::string const& stderr_handler_name = std::string{"stderr"})
  {
    // we just pass an empty ConsoleColours class, as we don't use colours for stderr.
    return _create_console_handler(stderr_handler_name, stderr, ConsoleColours{});
  }

  /**
   * Create a handler
   */
  template <typename THandler, typename... Args>
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> create_handler(std::string const& handler_name,
                                                                               Args&&... args)
  {
    // Protect shared access
    std::lock_guard<std::mutex> const lock{_mutex};

    // Try to insert it unless we failed it means we already had it
    auto const search = _handler_collection.find(handler_name);

    // First search if we have it and don't create it yet as this will call fopen
    if (search != _handler_collection.cend())
    {
      std::shared_ptr<Handler> handler = search->second.lock();
      if (handler)
      {
        return handler;
      }

        // recreate this handler
        if constexpr (std::is_base_of_v<StreamHandler, THandler>)
        {
          handler = std::make_shared<THandler>(handler_name, std::forward<Args>(args)...);
        }
        else
        {
          handler = std::make_shared<THandler>(std::forward<Args>(args)...);
        }

        search->second = handler;
        return handler;
    }

    // if first time add it
    std::shared_ptr<Handler> handler;
    if constexpr (std::is_base_of_v<StreamHandler, THandler>)
    {
      handler = std::make_shared<THandler>(handler_name, std::forward<Args>(args)...);
    }
    else
    {
      handler = std::make_shared<THandler>(std::forward<Args>(args)...);
    }

    _handler_collection.emplace(handler_name, handler);
    return handler;
  }

  /**
   * Get an existing a handler
   * @param handler_name the name of the handler
   * @throws std::runtime_error if the handler does not exist
   * @return a shared_ptr to the handler
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> get_handler(std::string const& handler_name) const
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

  /**
   * Subscribe a handler to the vector of active handlers so that the backend thread can see it
   * Called each time a new Logger instance is created. If the Handler already exists then it is not
   * added in the collection again
   * Objects that are added to _active_handlers_collection never get removed again
   * @param handler_to_insert a handler to add
   */
  void subscribe_handler(std::shared_ptr<Handler> const& handler_to_insert)
  {
    // Protect shared access
    std::lock_guard<std::mutex> const lock{_mutex};

    // Check if we already have this object
    auto const search =
      std::find_if(_active_handlers_collection.cbegin(), _active_handlers_collection.cend(),
                   [&handler_to_insert](std::weak_ptr<Handler> const& handler_elem)
                   {
                     std::shared_ptr<Handler> const elem = handler_elem.lock();
                     if (!elem)
                     {
                       return false;
                     }

                     return elem.get() == handler_to_insert.get();
                   });

    if (search == _active_handlers_collection.cend())
    {
      // we don't have this object so add it
      _active_handlers_collection.push_back(handler_to_insert);
    }
  }

  /**
   * Get a list of all the active subscribed handlers.
   * The list contains each handler only once regardless the amount of Logger instances using it
   * This is not used for logging by the backend but only in special cases when
   * e.g. it needs to iterate through all handlers for e.g. to flush
   * @param active_handlers_collection active handlers collection
   */
  void active_handlers(std::vector<std::weak_ptr<Handler>>& active_handlers_collection) const
  {
    // Protect shared access, we just use a lock here since this function is not used when logging
    // messages but only in special cases e.g. flushing
    std::lock_guard<std::mutex> const lock{_mutex};
    active_handlers_collection = _active_handlers_collection;
  }

  /**
   * Called by the backend worker thread only to remove any handlers that are not longer in use
   */
  void remove_unused_handlers()
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

private:
  QUILL_NODISCARD std::shared_ptr<Handler> _create_console_handler(std::string const& stream, FILE* file,
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
        auto const handler = reinterpret_cast<StreamHandler*>(handler_ptr.get());

        // if the handler with that name was already created check that the user didn't try to create it again passing a different file
        if (file == stdout && (handler->stream_handler_type() != StreamHandler::StreamHandlerType::Stdout))
        {
          QUILL_THROW(
            QuillError("Trying to insert an stdout handler again, but the handler already exists "
                       "as a different "
                       "file. Use an unique stream_handler name"));
        }

        if (file == stderr && (handler->stream_handler_type() != StreamHandler::StreamHandlerType::Stderr))
        {
          QUILL_THROW(
            QuillError("Trying to insert an stderr handler again, but the handler already exists "
                       "as a different "
                       "file. Use an unique stream_handler name"));
        }

        if (handler->stream_handler_type() == StreamHandler::StreamHandlerType::File)
        {
          QUILL_THROW(
            QuillError("Trying to insert an stdout/stderr handler, but the handler already exists. "
                       "Use an unique "
                       "stream_handler name"));
        }

        return handler_ptr;
      }

      // found a similar handler but with an out of date weak ptr, this is very rare to happen
      // as the backend logging thread is cleaning them
      // in that case allocate a new handler in that location
      handler_ptr = std::make_shared<ConsoleHandler>(stream, file, console_colours);
      search->second = handler_ptr;
      return handler_ptr;
    }

    // if first time add it
    auto handler_ptr = std::make_shared<ConsoleHandler>(stream, file, console_colours);
    _handler_collection.emplace(stream, handler_ptr);
    return handler_ptr;
  }

private:
  /**
   * All handlers that are currently owned by all Logger instances are registered here
   * Since the Logger instances share the same handlers, this collection contains unique handlers
   * @note Accessed by the frontend and the backend
   */
  std::vector<std::weak_ptr<Handler>> _active_handlers_collection;

  /**
   * Owns all created handlers. Each handler is identified by name
   * For Streamhandlers the name is the filename, they are stored per unique filename so
   * that we don't open_file the same file twice
   */
  std::unordered_map<std::string, std::weak_ptr<Handler>> _handler_collection;

  /** Use to lock both _active_handlers_collection and _file_handler_collection, mutable to have an active_handlers() const function */
  mutable std::mutex _mutex;
};
} // namespace detail
} // namespace quill