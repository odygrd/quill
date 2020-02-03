#pragma once

#include "quill/detail/utility/Spinlock.h"
#include "quill/handlers/StreamHandler.h"

#include <memory>
#include <unordered_map>

namespace quill
{
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

  StreamHandler* stdout_streamhandler();

  StreamHandler* stderr_streamhandler();

  StreamHandler* filehandler(std::string const& filename, std::string const& mode = std::string{"a"});

  /**
   * Subscribe a handler to the vector of active handlers so that the backend thread can see it
   * Called each time a new Logger instance is created. If the Handler already exists then it is not
   * added in the collection again
   * Objects that are added to _active_handlers_collection never get removed again
   * @param handler_to_insert
   */
  void subscribe_handler(Handler* handler_to_insert);

  /**
   * Get a list of all the active subscribed handlers.
   * The list contains each handler only once regardless the amount of Logger instances using it
   * This is not used for logging by the backend but only in special cases when
   * e.g. it needs to iterate through all handlers for e.g. to flush
   */
  std::vector<Handler*> active_handlers() const;

  // TODO: Remove handlers. e.g when we are set_default_logger to FILE* and stdout remains in our
  // list Check if no other logger is using it first

private:
  StreamHandler* _create_streamhandler(std::string const& stream);

private:
  /** mutable to have an active_handlers() const function */
  mutable Spinlock _spinlock;

  /**
   * All related to files Streamhandlers, stored per unique filename so that we don't open the same file twice
   */
  std::unordered_map<std::string, std::unique_ptr<StreamHandler>> _file_handler_collection;

  /**
   * All handlers that are currently owned by all Logger instances are registered here
   * Since the Logger instances share the same handlers, this collection contains unique handlers
   * @note Accessed by the frontend and the backend
   */
  std::vector<Handler*> _active_handlers_collection;
};
} // namespace detail
} // namespace quill