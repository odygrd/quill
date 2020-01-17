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
   * Creates a new logger with default log level info or returns an existing logger with it's
   * cached log level if the logger already exists
   * @param logger_name The name of the logger or empty string for default logger
   * @return a Logger object
   */
  [[nodiscard]] Logger* get_logger(std::string const& logger_name = std::string{}) const;

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