#pragma once

#include "quill/detail/LoggerCollection.h"
#include "quill/detail/LoggingWorker.h"
#include "quill/detail/ThreadContextCollection.h"

namespace quill::detail
{

class LogManager
{
public:
  /**
   * Ctor
   */
  LogManager() = default;

  /**
   * Deleted
   */
  LogManager(LogManager const&) = delete;
  LogManager& operator=(LogManager const&) = delete;

  /**
   * Returns an existing logger with it's cached properties if the logger already exists
   * @param logger_name The name of the logger or empty string for default logger
   * @return a Logger object
   */
  [[nodiscard]] Logger* get_logger(std::string const& logger_name = std::string{}) const;

  /**
   * Creates aa new level using the sink and the formatter as set for the default logger
   * @param logger_name
   * @param sink
   * @return
   */
  [[nodiscard]] Logger* create_logger(std::string const& logger_name)
  {
    return _logger_collection.create_logger(logger_name);
  }

  /**
   * Creates a new logger with default log level info
   * @tparam TSink
   * @tparam TSinkArgs
   * @param logger_name
   * @param sink_args
   * @return a pointer to the created logger
   */
  [[nodiscard]] Logger* create_logger(std::string const& logger_name, std::unique_ptr<SinkBase> sink)
  {
    return _logger_collection.create_logger(logger_name, std::move(sink));
  }

  // TODO:: how to change the format of the default logger?

  /**
   * Starts the logging worker thread
   */
  void start_logging_worker();

  /**
   * Stops the logging worker thread
   */
  void stop_logging_worker();

private:
  ThreadContextCollection _thread_context_collection;
  LoggerCollection _logger_collection{_thread_context_collection};
  LoggingWorker _logging_worker{_thread_context_collection};
};
} // namespace quill::detail