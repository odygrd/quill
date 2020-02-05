#pragma once

#include "quill/detail/BackendWorker.h"
#include "quill/detail/Config.h"
#include "quill/detail/HandlerCollection.h"
#include "quill/detail/LoggerCollection.h"
#include "quill/detail/ThreadContextCollection.h"

namespace quill
{
namespace detail
{
/**
 * Provides access to common collection class that are used by both the frontend and the backend
 * components of the logging system
 * There should only be only active active instance of this class which is achieved by the
 * LogManagerSingleton
 */
class LogManager
{
public:
  /**
   * Constructor
   */
  explicit LogManager(Config const& config);

  /**
   * Deleted
   */
  LogManager(LogManager const&) = delete;
  LogManager& operator=(LogManager const&) = delete;

  /**
   * @return A reference to the logger collection
   */
  [[nodiscard]] LoggerCollection& logger_collection() noexcept { return _logger_collection; }

  /**
   * @return A reference to the handler collection
   */
  [[nodiscard]] HandlerCollection& handler_collection() noexcept { return _handler_collection; }

  /**
   * Blocks the caller thread until all log messages until the current timestamp are flushed
   *
   * The backend thread will flush all loggers and all handlers up to the point (timestamp) that
   * this function was called.
   */
  void flush();

  /**
   * Starts the backend worker thread
   */
  void start_backend_worker();

  /**
   * Stops the backend worker thread
   */
  void stop_backend_worker();

private:
  Config const& _config;                              // TODO: Move ownership here
  HandlerCollection _handler_collection;              /** 2 Cache lines **/
  ThreadContextCollection _thread_context_collection; /** 1 Cache line **/
  LoggerCollection _logger_collection{_thread_context_collection, _handler_collection}; /** 2 Cache lines **/
  BackendWorker _backend_worker{_config, _thread_context_collection, _logger_collection,
                                _handler_collection}; /** 1 Cache line **/
};
} // namespace detail
} // namespace quill