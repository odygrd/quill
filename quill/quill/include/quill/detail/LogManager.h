/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

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
  LogManager() = default;

  /**
   * Deleted
   */
  LogManager(LogManager const&) = delete;
  LogManager& operator=(LogManager const&) = delete;

  /**
   * @return A reference to the current config
   */
  QUILL_NODISCARD detail::Config& config() noexcept { return _config; }

  /**
   * @return A reference to the logger collection
   */
  QUILL_NODISCARD LoggerCollection& logger_collection() noexcept { return _logger_collection; }

  /**
   * @return A reference to the handler collection
   */
  QUILL_NODISCARD HandlerCollection& handler_collection() noexcept { return _handler_collection; }

  /**
   * @return A reference to the thread context collection
   */
  QUILL_NODISCARD ThreadContextCollection& thread_context_collection() noexcept
  {
    return _thread_context_collection;
  }

  /**
   * @return The current process id
   */
  QUILL_NODISCARD std::string const& process_id() noexcept { return _process_id; }

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
  QUILL_ATTRIBUTE_COLD void start_backend_worker();

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop_backend_worker();

private:
  Config _config;
  HandlerCollection _handler_collection;
  ThreadContextCollection _thread_context_collection{_config};
  LoggerCollection _logger_collection{_thread_context_collection, _handler_collection};
  BackendWorker _backend_worker{_config, _thread_context_collection, _handler_collection};
  std::string _process_id{fmt::format_int(get_process_id()).str()};
};
} // namespace detail
} // namespace quill