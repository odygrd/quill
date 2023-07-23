/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"  // for QUILL_NODISCARD, QUILL_ATT...
#include "quill/handlers/ConsoleHandler.h" // for ConsoleColours
#include "quill/handlers/FileHandler.h"    // for FilenameAppend
#include "quill/handlers/StreamHandler.h"  // for StreamHandler
#include <chrono>                          // for hours, minutes
#include <cstddef>                         // for size_t
#include <memory>                          // for allocator, unique_ptr
#include <memory>
#include <mutex>  // for lock_guard
#include <string> // for string, hash
#include <type_traits>
#include <unordered_map> // for unordered_map
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
    ConsoleColours const& console_colours = ConsoleColours{});

  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> stderr_console_handler(
    std::string const& stderr_handler_name = std::string{"stderr"});

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
      else
      {
        // recreate this handler
        if constexpr (std::is_base_of<StreamHandler, THandler>::value)
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
    }

    // if first time add it
    std::shared_ptr<Handler> handler;
    if constexpr (std::is_base_of<StreamHandler, THandler>::value)
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
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> get_handler(std::string const& handler_name);

  /**
   * Subscribe a handler to the vector of active handlers so that the backend thread can see it
   * Called each time a new Logger instance is created. If the Handler already exists then it is not
   * added in the collection again
   * Objects that are added to _active_handlers_collection never get removed again
   * @param handler_to_insert a handler to add
   */
  void subscribe_handler(std::shared_ptr<Handler> const& handler_to_insert);

  /**
   * Get a list of all the active subscribed handlers.
   * The list contains each handler only once regardless the amount of Logger instances using it
   * This is not used for logging by the backend but only in special cases when
   * e.g. it needs to iterate through all handlers for e.g. to flush
   * @return a vector containing all the active handlers
   */
  QUILL_NODISCARD std::vector<std::weak_ptr<Handler>> active_handlers() const;

  /**
   * Called by the backend worker thread only to remove any handlers that are not longer in use
   */
  void remove_unused_handlers();

  // TODO: Remove handlers. e.g when we are set_default_logger to FILE* and stdout remains in our
  // list Check if no other logger is using it first

private:
  QUILL_NODISCARD std::shared_ptr<Handler> _create_console_handler(std::string const& stream, FILE* file,
                                                                   ConsoleColours const& console_colours);

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