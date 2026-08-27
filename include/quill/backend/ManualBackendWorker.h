/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/BackendOptions.h"
#include "quill/backend/BackendWorker.h"
#include <chrono>
#include <limits>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * This class can be used when you want to run the backend worker on your own thread.
 *
 * Threading contract:
 * - By default, the thread that calls `init()` must perform all polling and call `shutdown()`.
 *   This is the usual and recommended mode.
 * - In the default mode, the thread running `ManualBackendWorker` may log, but it must not use any
 *   path that waits for the backend to flush its own queue.
 *   In particular, it must not call `logger->flush_log()` or `Frontend::remove_logger_blocking()`.
 *   If a logger has immediate flush enabled, the implicit flush is skipped for log calls from this
 *   thread.
 * - In the default mode, the thread that calls `init()` must also call `shutdown()` explicitly
 *   before it exits. Do not rely on the destructor to perform shutdown for you.
 *
 * `init(options, true)` enables thread migration for externally managed executors and task pools.
 * In that mode, successive worker operations may run on different threads, but the caller must
 * externally serialize them, and completion of `init()` and each worker operation must
 * happen-before the next operation starts. Quill does not track which thread is running the backend
 * in this mode, so backend-thread deadlock checks are disabled and signal-handler logging and
 * flushing are not supported.
 */
class ManualBackendWorker
{
public:
  explicit ManualBackendWorker(detail::BackendWorker* backend_worker)
    : _backend_worker(backend_worker)
  {
  }

  ManualBackendWorker(ManualBackendWorker const&) = delete;
  ManualBackendWorker& operator=(ManualBackendWorker const&) = delete;
  ManualBackendWorker(ManualBackendWorker&&) = delete;
  ManualBackendWorker& operator=(ManualBackendWorker&&) = delete;

  ~ManualBackendWorker()
  {
    // Preserve legacy behavior for callers that forgot explicit shutdown().
    shutdown();
  }

  /**
   * @brief Initializes the ManualBackendWorker with the specified backend options.
   *
   * This function must be called before any other functions in this class. It configures the backend worker
   * for manual operation, disabling certain options that are incompatible with manual control.
   *
   * @param options The `BackendOptions` to configure the backend worker.
   * @param allow_thread_migration When `false` (the default and recommended mode), the thread
   * calling `init()` must perform all polling and call `shutdown()`. When `true`, successive calls
   * to `poll_one()`, `poll()`, and `shutdown()` may run on different threads. The caller must
   * externally serialize those operations so completion of `init()` and each operation happens-before
   * invocation of the next. Quill does not track a backend thread in this mode, and
   * `Backend::get_thread_id()` returns zero. Code executed by the backend, including hooks, sinks,
   * and error notifiers, must not call `flush_log()`, `Frontend::remove_logger_blocking()`, or
   * otherwise block waiting for the backend. Quill's signal handler cannot log or flush in
   * thread-migration mode.
   */
  void init(BackendOptions options, bool allow_thread_migration = false)
  {
    QUILL_ASSERT(!_started, "ManualBackendWorker::init() must not be called more than once");

    options.sleep_duration = std::chrono::nanoseconds{0};
    options.enable_yield_when_idle = false;
    _backend_worker->_init(options);
    _allow_thread_migration = allow_thread_migration;

    if (_allow_thread_migration)
    {
      // Migratable mode has no persistent backend-thread identity. This also ensures that the
      // thread that called init() is treated as an ordinary producer after init() returns.
      _backend_worker->_clear_backend_thread_flag();
      _backend_worker->_worker_thread_id.store(0);
    }

    _started = true;
  }

  /**
   * Flushes remaining frontend queues and marks the manual backend worker as stopped.
   *
   * Unless thread migration was enabled during `init()`, this function must be called from the same
   * thread that called `init()`. It performs the same shutdown work that the automatic backend
   * thread executes during `stop()`. Call this explicitly after all polling has finished. Do not
   * rely on the destructor to do this for you.
   */
  void shutdown()
  {
    if (!_started)
    {
      return;
    }

    QUILL_ASSERT(
      _allow_thread_migration || (_backend_worker->_worker_thread_id.load() == detail::get_thread_id()),
      "ManualBackendWorker::shutdown() must be called from the same thread that called init() "
      "unless thread migration is enabled");

    _backend_worker->_exit();
    _backend_worker->_worker_thread_id.store(0);
    _started = false;
  }

  /**
   * Polls all thread-local SPSC queues and caches the log statements, processing and
   * writing the log statement with the minimum timestamp to the corresponding output sinks.
   *
   * This function should be called periodically by the thread to process and dispatch log entries.
   * It assumes that the `init()` function has been called first to properly configure the backend worker.
   *
   */
  void poll_one()
  {
    QUILL_ASSERT(_started, "ManualBackendWorker::poll_one() requires init() to be called first");
    QUILL_ASSERT(
      _backend_worker->_options.sleep_duration.count() == 0,
      "ManualBackendWorker::poll_one() requires init() to be called first with sleep_duration = 0");
    QUILL_ASSERT(_backend_worker->_options.enable_yield_when_idle == false,
                 "ManualBackendWorker::poll_one() requires init() to be called first with "
                 "enable_yield_when_idle = false");
    QUILL_ASSERT(
      _allow_thread_migration || (_backend_worker->_worker_thread_id.load() == detail::get_thread_id()),
      "ManualBackendWorker::poll_one() must be called from the thread that called init() unless "
      "thread migration is enabled");

    QUILL_TRY { _backend_worker->_poll(); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH(std::exception const& e)
    {
      _backend_worker->_notify_error(_backend_worker->_options.error_notifier, e.what());
    }
    QUILL_CATCH_ALL()
    {
      _backend_worker->_notify_error(_backend_worker->_options.error_notifier,
                                     std::string{"Caught unhandled exception."});
    }
#endif
  }

  /**
   * Continuously polls the backend worker until all queues are empty.
   *
   * This function keeps polling until all frontend queues and cached transit events are processed.
   */
  void poll()
  {
    while (!_backend_worker->_check_frontend_queues_and_cached_transit_events_empty())
    {
      poll_one();
    }
  }

  /**
   * This function behaves like `poll()` but will stop polling if the specified timeout duration is reached before
   * all queues are emptied. This can be useful when you want to limit the time spent processing log statements.
   */
  void poll(std::chrono::microseconds timeout)
  {
    auto const start = std::chrono::steady_clock::now();
    while (!_backend_worker->_check_frontend_queues_and_cached_transit_events_empty())
    {
      poll_one();

      auto const now = std::chrono::steady_clock::now();

      if ((now - start) > timeout)
      {
        break;
      }
    }
  }

private:
  detail::BackendWorker* _backend_worker;
  bool _started{false};
  bool _allow_thread_migration{false};
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
