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
#include <mutex>                           // for lock_guard
#include <string>                          // for string, hash
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

  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD StreamHandler* stdout_console_handler(
    std::string const& stdout_handler_name = std::string{"stdout"},
    ConsoleColours const& console_colours = ConsoleColours{});

  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD StreamHandler* stderr_console_handler(
    std::string const& stderr_handler_name = std::string{"stderr"});

  /**
   * Create a handler. This overload is used for any handlers deriving from StreamHandler.
   * For StreamHandler we pass the handler_name as the filename of the handler
   */
  template <typename THandler, typename... Args>
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::enable_if_t<std::is_base_of<StreamHandler, THandler>::value, StreamHandler*> create_handler(
    std::string const& handler_name, Args&&... args)
  {
    // Protect shared access
    std::lock_guard<std::mutex> const lock{_mutex};

    // Try to insert it unless we failed it means we already had it
    auto const search = _handler_collection.find(handler_name);

    // First search if we have it and don't call make_unique yet as this will call fopen
    if (search != _handler_collection.cend())
    {
      return reinterpret_cast<StreamHandler*>((*search).second.get());
    }

    // if first time add it
    auto emplace_result = _handler_collection.emplace(
      handler_name, std::make_unique<THandler>(handler_name.data(), std::forward<Args>(args)...));

    // we know that THandler derives from StreamHandler
    return reinterpret_cast<StreamHandler*>((*emplace_result.first).second.get());
  }

  /**
   * Create a handler. Any handler that is not based on StreamHandler
   */
  template <typename THandler, typename... Args>
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::enable_if_t<!std::is_base_of<StreamHandler, THandler>::value, Handler*> create_handler(
    std::string const& handler_name, Args&&... args)
  {
    // Protect shared access
    std::lock_guard<std::mutex> const lock{_mutex};

    // Try to insert it unless we failed it means we already had it
    auto const search = _handler_collection.find(handler_name);

    // First search if we have it and don't call make_unique yet as this will call fopen
    if (search != _handler_collection.cend())
    {
      return (*search).second.get();
    }

    // if first time add it
    auto emplace_result =
      _handler_collection.emplace(handler_name, std::make_unique<THandler>(std::forward<Args>(args)...));

    return (*emplace_result.first).second.get();
  }

  /**
   * Subscribe a handler to the vector of active handlers so that the backend thread can see it
   * Called each time a new Logger instance is created. If the Handler already exists then it is not
   * added in the collection again
   * Objects that are added to _active_handlers_collection never get removed again
   * @param handler_to_insert a handler to add
   */
  void subscribe_handler(Handler* handler_to_insert);

  /**
   * Get a list of all the active subscribed handlers.
   * The list contains each handler only once regardless the amount of Logger instances using it
   * This is not used for logging by the backend but only in special cases when
   * e.g. it needs to iterate through all handlers for e.g. to flush
   * @return a vector containing all the active handlers
   */
  QUILL_NODISCARD std::vector<Handler*> active_handlers() const;

  // TODO: Remove handlers. e.g when we are set_default_logger to FILE* and stdout remains in our
  // list Check if no other logger is using it first

private:
  QUILL_NODISCARD StreamHandler* _create_console_handler(std::string const& stream, FILE* file,
                                                         ConsoleColours const& console_colours);

private:
  /**
   * All handlers that are currently owned by all Logger instances are registered here
   * Since the Logger instances share the same handlers, this collection contains unique handlers
   * @note Accessed by the frontend and the backend
   */
  std::vector<Handler*> _active_handlers_collection;

  /**
   * Owns all created handlers. Each handler is identified by name
   * For Streamhandlers the name is the filename, they are stored per unique filename so
   * that we don't open_file the same file twice
   */
  std::unordered_map<std::string, std::unique_ptr<Handler>> _handler_collection;

  /** Use to lock both _active_handlers_collection and _file_handler_collection, mutable to have an active_handlers() const function */
  mutable std::mutex _mutex;
};
} // namespace detail
} // namespace quill