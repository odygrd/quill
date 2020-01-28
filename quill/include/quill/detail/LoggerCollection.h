#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "quill/Logger.h"

namespace quill::detail
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
  explicit LoggerCollection(ThreadContextCollection& thread_context_collection);

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
   * cached log levels and sinks if the logger already exists
   * @param logger_name The name of the logger or empty for the default logger
   * @note this function is slow, consider calling it only once and store the pointer to the logger
   * @return a Logger object or the default logger is logger_name is empty
   */
  [[nodiscard]] Logger* get_logger(std::string const& logger_name = std::string{}) const;

  /**
   * Create a new logger using the same sink and formatter as the default logger
   * @param logger_name
   * @return
   */
  [[nodiscard]] Logger* create_logger(std::string logger_name);

  /**
   * Creates a new logger
   * @tparam TSink The type of the sink of this ogger
   * @tparam TSinkArgs
   * @param logger_name logger name to be added
   * @param sink_args Sink constructor arguments
   * @return
   */
  [[nodiscard]] Logger* create_logger(std::string logger_name, std::unique_ptr<SinkBase> sink);

  /**
   * Create a new logger with multiple sinks
   * @tparam TSinks
   * @param logger_name
   * @param sink
   * @param sinks
   * @return
   */
  template <typename... TSinks>
  [[nodiscard]] Logger* create_logger(std::string logger_name, std::unique_ptr<SinkBase> sink, TSinks&&... sinks)
  {
    // Create a vector of unique pointers to sinks
    std::vector<std::unique_ptr<SinkBase>> sinks_collection;
    _make_sinks_collection(sinks_collection, std::move(sink), std::forward<TSinks>(sinks)...);
    return _create_logger(std::move(logger_name), std::move(sinks_collection));
  }

  /**
   * Set a custom default logger with a single sink
   * @param sink
   */
  void set_custom_default_logger(std::unique_ptr<SinkBase> sink);

  /**
   * Set a custom default logger with multple sinks
   * @tparam TSinks
   * @param sink
   * @param sinks
   */
  template <typename... TSinks>
  void set_custom_default_logger(std::unique_ptr<SinkBase> sink, TSinks&&... sinks)
  {
    // Create a vector of unique pointers to sinks
    std::vector<std::unique_ptr<SinkBase>> sinks_collection;
    _make_sinks_collection(sinks_collection, std::move(sink), std::forward<TSinks>(sinks)...);
    _set_custom_default_logger(std::move(sinks_collection));
  }

private:
  /**
   * End of recursion
   * @param sinks_collection
   * @param sink
   */
  static void _make_sinks_collection(std::vector<std::unique_ptr<SinkBase>>& sinks_collection,
                                     std::unique_ptr<SinkBase> sink)
  {
    sinks_collection.push_back(std::move(sink));
  }

  /**
   * Recursively create a vector of sinks to pass it to a new logger constructor
   * @tparam TSinks
   * @param sinks_collection
   * @param sink
   * @param sinks
   */
  template <typename... TSinks>
  static void _make_sinks_collection(std::vector<std::unique_ptr<SinkBase>>& sinks_collection,
                                     std::unique_ptr<SinkBase> sink,
                                     TSinks&&... sinks)
  {
    sinks_collection.push_back(std::move(sink));
    _make_sinks_collection(sinks_collection, std::forward<TSinks>(sinks)...);
  }

  /**
   * Sets a custom logger with multiple sinks
   * @param sink
   */
  void _set_custom_default_logger(std::vector<std::unique_ptr<SinkBase>> sink_collection);

  /**
   * Create a logger with multiple sinks
   * @param logger_name
   * @param sink
   * @return
   */
  [[nodiscard]] Logger* _create_logger(std::string logger_name,
                                       std::vector<std::unique_ptr<SinkBase>> sinks_collection);

private:
  ThreadContextCollection& _thread_context_collection; /**< We need to pass this to each logger */
  Logger* _default_logger{nullptr}; /**< A pointer to the default logger to avoid lookup */

  /**<
   * In order to allow const functions having access to get_logger to get a logger everything
   * is mutable
   */
  mutable std::recursive_mutex _mutex; /**< Thread safe access to logger map */
  mutable std::unordered_map<std::string, std::unique_ptr<Logger>> _logger_name_map; /**< map from logger name to the actual logger */
};

} // namespace quill::detail