/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Logger.h"                        // for Logger
#include "quill/detail/misc/Attributes.h"        // for QUILL_ATTRIBUTE_COLD
#include "quill/detail/misc/Common.h"            // for CACHELINE_SIZE
#include "quill/detail/misc/RecursiveSpinlock.h" // for RecursiveSpinlock
#include <initializer_list>                      // for initializer_list
#include <memory>                                // for unique_ptr
#include <string>                                // for string, hash
#include <unordered_map>                         // for unordered_map

namespace quill
{
/** forward declarations **/
class Handler;

namespace detail
{
/** forward declarations **/
class ThreadContextCollection;
class HandlerCollection;

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
  ~LoggerCollection() = default;

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
  QUILL_NODISCARD Logger* get_logger(char const* logger_name = nullptr) const;

  /**
   * Create a new logger using the same handlers and formatter as the default logger
   * @param logger_name the name of the logger to add
   * @return a pointer to the logger
   */
  Logger* create_logger(char const* logger_name);

  /**
   * Creates a new logger
   * @param logger_name the name of the logger to add
   * @param handler The handler for the logger
   * @return a pointer to the logger
   */
  Logger* create_logger(char const* logger_name, Handler* handler);

  /**
   * Create a new logger with multiple handler
   * @param logger_name the name of the logger to add
   * @param handlers An initializer list of pointers to handlers that will be now used as a default handler
   * @return a pointer to the logger
   */
  Logger* create_logger(char const* logger_name, std::initializer_list<Handler*> handlers);

  /**
   * Set a custom default logger with a single handler
   * @param handler A pointer to a handler that will be now used as a default handler
   */
  QUILL_ATTRIBUTE_COLD void set_default_logger_handler(Handler* handler);

  /**
   * Set a custom default logger with multiple handlers
   * @param handlers A vector of pointers to handlers that will be now used as a default handler
   */
  QUILL_ATTRIBUTE_COLD void set_default_logger_handler(std::initializer_list<Handler*> handlers);

  /**
   * Used internally only to enable console colours on "stdout" default console handler
   * which is created by default in this object constructor.
   * This is meant to called before quill:start() and that is checked internally before calling
   * this function.
   */
  QUILL_ATTRIBUTE_COLD void enable_console_colours() noexcept;

private:
  ThreadContextCollection& _thread_context_collection; /**< We need to pass this to each logger */
  HandlerCollection& _handler_collection;              /** Collection of al handlers **/
  Logger* _default_logger{nullptr}; /**< A pointer to the default logger to avoid lookup */

  /** We can not avoid having less than 2 cache lines here, so we will just align the lock and the logger map on the same cache line as they are used together anyway */
  alignas(detail::CACHELINE_SIZE) mutable RecursiveSpinlock _spinlock; /**< Thread safe access to logger map, Mutable to have a const get_logger() function  */
  std::unordered_map<std::string, std::unique_ptr<Logger>> _logger_name_map; /**< map from logger name to the actual logger */
};

} // namespace detail
} // namespace quill