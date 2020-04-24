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

#include "quill/detail/misc/Spinlock.h"
#include "quill/detail/record/CommandRecord.h"

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
  void flush()
  {
    if (!_backend_worker.is_running())
    {
      // Backend worker needs to be running, otherwise we are stuck for ever waiting
      return;
    }

    // Create an atomic variable
    std::atomic<bool> backend_thread_flushed{false};

    // notify will be invoked done the backend thread when this message is processed
    auto notify_callback = [&backend_thread_flushed]() {
      // When the backend thread is done flushing it will set the flag to true
      backend_thread_flushed.store(true);
    };

    using log_record_t = detail::CommandRecord;
    _thread_context_collection.local_thread_context()->spsc_queue().emplace<log_record_t>(notify_callback);

    // The caller thread keeps checking the flag until the backend thread flushes
    do
    {
      // wait
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    } while (!backend_thread_flushed.load());
  }

  /**
   * Starts the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void inline start_backend_worker() { _backend_worker.run(); }

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop_backend_worker() { _backend_worker.stop(); }

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