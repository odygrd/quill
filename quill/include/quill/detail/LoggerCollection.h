#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "quill/Logger.h"
#include "quill/detail/HandlerCollection.h"
#include "quill/detail/utility/RecursiveSpinlock.h"

namespace quill
{
namespace detail
{
/** forward declarations **/
class ThreadContextCollection;

/**
 * Contains all created loggers
 */
class LoggerCollection
{
public:
  /**
   * Constructor
   */
  LoggerCollection(ThreadContextCollection& thread_context_collection, HandlerCollection& handler_collection);

  /**
   * Destructor
   */
  virtual ~LoggerCollection() = default;

  /**
   * Deleted
   */
  LoggerCollection(LoggerCollection const&) = delete;
  LoggerCollection& operator=(LoggerCollection const&) = delete;

  /**
   * Creates a new logger with default log level info or returns an existing logger with it's
   * cached log levels and handlers if the logger already exists
   * @param logger_name The name of the logger or empty for the default logger
   * @note this function is slow, consider calling it only once and store the pointer to the logger
   * @return a Logger object or the default logger is logger_name is empty
   */
  [[nodiscard]] Logger* get_logger(std::string const& logger_name = std::string{}) const;

  /**
   * Create a new logger using the same handlers and formatter as the default logger
   * @param logger_name
   * @return
   */
  [[nodiscard]] Logger* create_logger(std::string logger_name);

  /**
   * Creates a new logger
   * @param logger_name logger name to be added
   * @param handler The handler of the loggfer
   * @return
   */
  [[nodiscard]] Logger* create_logger(std::string logger_name, Handler* handler);

  /**
   * Create a new logger with multiple handler
   * @param logger_name
   * @param handlers
   * @return
   */
  [[nodiscard]] Logger* create_logger(std::string logger_name, std::initializer_list<Handler*> handlers);

  /**
   * Set a custom default logger with a single handler
   * @param handler
   */
  void set_default_logger_handler(Handler* handler);

  /**
   *  Set a custom default logger with multiple handlers
   * @param handlers
   */
  void set_default_logger_handler(std::initializer_list<Handler*> handlers);

private:
  ThreadContextCollection& _thread_context_collection; /**< We need to pass this to each logger */
  HandlerCollection& _handler_collection;              /** Collection of al handlers **/
  Logger* _default_logger{nullptr}; /**< A pointer to the default logger to avoid lookup */

  /** Mutable to have a const get_logger() function */
  mutable RecursiveSpinlock _spinlock; /**< Thread safe access to logger map */
  std::unordered_map<std::string, std::unique_ptr<Logger>> _logger_name_map; /**< map from logger name to the actual logger */

  /**
   * A cache to the loggers in _logger_name_map.
   *
   * @note Accessed strictly only by the backend thread
   */
  std::vector<LoggerDetails const*> _logger_cache;
};
} // namespace detail
} // namespace quill