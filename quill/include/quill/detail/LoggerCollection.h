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
   * Deleted
   */
  LoggerCollection(LoggerCollection const&) = delete;
  LoggerCollection& operator=(LoggerCollection const&) = delete;

  /**
   * Creates a new logger with default log level info or returns an existing logger with it's
   * cached log level if the logger already exists
   * @param logger_name The name of the logger or empty for the default logger
   * @note this function is slow, consider calling it only once and store the pointer to the logger
   * @return a Logger object or the default logger is logger_name is empty
   */
  [[nodiscard]] Logger* get_logger(std::string const& logger_name = std::string{}) const;

  /**
   * Creates a new logger
   * @tparam TSink The type of the sink of this ogger
   * @tparam TSinkArgs
   * @param logger_name logger name to be added
   * @param sink_args Sink constructor arguments
   * @return
   */
  template <typename TSink, typename... TSinkArgs>
  [[nodiscard]] Logger* create_logger(std::string const& logger_name, TSinkArgs&&... sink_args);

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