/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Config.h"
#include "quill/Logger.h" // for Logger
#include "quill/clock/TimestampClock.h"
#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_COLD
#include "quill/detail/misc/Common.h"
#include <functional>
#include <initializer_list> // for initializer_list
#include <memory>           // for unique_ptr
#include <mutex>
#include <string>        // for string, hash
#include <unordered_map> // for unordered_map

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
  LoggerCollection(Config const& config, ThreadContextCollection& thread_context_collection,
                   HandlerCollection& handler_collection);

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
   * @param logger_name The name of the logger or empty for the root logger
   * @note this function is slow, consider calling it only once and store the pointer to the logger
   * @return a Logger object or the root logger is logger_name is empty
   */
  QUILL_NODISCARD Logger* get_logger(char const* logger_name = nullptr) const;

  /**
   * Returns all existing loggers and the pointers to them
   * @return a map logger_name -> logger*
   */
  QUILL_NODISCARD std::unordered_map<std::string, Logger*> get_all_loggers() const;

  /**
   * Create a new logger using the same handlers and formatter as the root logger
   * @param logger_name the name of the logger to add
   * @param timestamp_clock_type timestamp clock type
   * @param timestamp_clock timestamp clock
   * @return a pointer to the logger
   */
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, TimestampClockType timestamp_clock_type,
                                        TimestampClock* timestamp_clock);

  /**
   * Creates a new logger
   * @param logger_name the name of the logger to add
   * @param handler The handler for the logger
   * @param timestamp_clock_type timestamp clock type
   * @param timestamp_clock timestamp clock
   * @return a pointer to the logger
   */
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, std::shared_ptr<Handler> handler,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock);

  /**
   * Create a new logger with multiple handler
   * @param logger_name the name of the logger to add
   * @param handlers An initializer list of pointers to handlers that will be now used as a default handler
   * @param timestamp_clock_type timestamp clock type
   * @param timestamp_clock timestamp clock
   * @return a pointer to the logger
   */
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                        std::initializer_list<std::shared_ptr<Handler>> handlers,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock);

  /**
   * Create a new logger with multiple handler
   * @param logger_name the name of the logger to add
   * @param handlers An initializer list of pointers to handlers that will be now used as a default handler
   * @param timestamp_clock_type timestamp clock type
   * @param timestamp_clock timestamp clock
   * @return a pointer to the logger
   */
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                        std::vector<std::shared_ptr<Handler>> handlers,
                                        TimestampClockType timestamp_clock_type, TimestampClock* timestamp_clock);

  /**
   * Marks a logger for deletion. The logger will asynchronously be removed by the logging thread
   */
  void remove_logger(Logger* logger);

  /**
   * Used internally only to enable console colours on "stdout" default console handler
   * which is created by default in this object constructor.
   * This is meant to called before quill:start() and that is checked internally before calling
   * this function.
   */
  QUILL_ATTRIBUTE_COLD void enable_console_colours() noexcept;

  /**
   * Get the root logger pointer
   * @return root logger ptr
   */
  QUILL_NODISCARD Logger* root_logger() const noexcept;

  /**
   * Creates or resets the root logger
   */
  QUILL_ATTRIBUTE_COLD void create_root_logger();

  /**
   * Called by the backend worker thread only to remove any loggers that are marked as
   * invalidated
   * @return true if loggers were removed
   */
  QUILL_NODISCARD bool remove_invalidated_loggers(std::function<bool(void)> const& check_queues_empty);

private:
  Config const& _config;
  ThreadContextCollection& _thread_context_collection; /**< We need to pass this to each logger */
  HandlerCollection& _handler_collection;              /** Collection of al handlers **/
  Logger* _root_logger{nullptr}; /**< A pointer to the root logger to avoid lookup */
  mutable std::recursive_mutex _rmutex; /**< Thread safe access to logger map, Mutable to have a const get_logger() function  */
  std::unordered_map<std::string, std::unique_ptr<Logger>> _logger_name_map; /**< map from logger name to the actual logger */
  std::atomic<bool> _has_invalidated_loggers{false};
};

} // namespace detail
} // namespace quill