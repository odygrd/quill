/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/detail/Config.h"
#include "quill/detail/HandlerCollection.h"
#include "quill/detail/LoggerCollection.h"
#include "quill/detail/SignalHandler.h" // for init_signal_handler
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/backend/BackendWorker.h"
#include "quill/detail/events/FlushEvent.h"
#include "quill/detail/misc/Spinlock.h"
#include <mutex> // for call_once, once_flag

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
  QUILL_NODISCARD std::string const& process_id() const noexcept { return _process_id; }

  /**
   * @return The current process id
   */
  QUILL_NODISCARD uint32_t backend_worker_thread_id() const noexcept
  {
    return _backend_worker.thread_id();
  }

  /**
   * Blocks the caller thread until all log messages until the current timestamp are flushed
   *
   * The backend thread will flush all loggers and all handlers up to the point (timestamp) that
   * this function was called.
   */
  void flush()
  {
    if ((!_backend_worker.is_running()) || (get_thread_id() == _backend_worker.thread_id()))
    {
      // 1. Backend worker needs to be running, otherwise we are stuck for ever waiting
      // 2. self-protection, backend_worker is not able to call this.
      return;
    }

    // Create an atomic variable
    std::atomic<bool> backend_thread_flushed{false};

    // notify will be invoked done the backend thread when this message is processed
    auto notify_callback = [&backend_thread_flushed]()
    {
      // When the backend thread is done flushing it will set the flag to true
      backend_thread_flushed.store(true);
    };

    using event_t = detail::FlushEvent;

#if defined(QUILL_USE_BOUNDED_QUEUE)
    // emplace to the spsc queue owned by the ctx, we never drop the flush message
    bool emplaced{false};
    do
    {
      emplaced =
        _thread_context_collection.local_thread_context()->event_spsc_queue().try_emplace<event_t>(notify_callback);
    } while (!emplaced);
#else
    _thread_context_collection.local_thread_context()->event_spsc_queue().emplace<event_t>(notify_callback);
#endif

    // The caller thread keeps checking the flag until the backend thread flushes
    do
    {
      // wait
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    } while (!backend_thread_flushed.load());
  }

  /**
   * Starts the backend worker thread.
   */
  QUILL_ATTRIBUTE_COLD void inline start_backend_worker(bool with_signal_handler,
                                                        std::initializer_list<int> const& catchable_signals)
  {
    // protect init to be called only once
    std::call_once(_start_init_once_flag,
                   [this, with_signal_handler, catchable_signals]()
                   {
                     if (with_signal_handler)
                     {
#if defined(_WIN32)
                       (void)catchable_signals;
                       init_exception_handler();
#else
                       // block all signals before spawning the backend worker thread
                       // note: we just assume that std::thread is implemented using posix threads
                       // or this won't have any effect
                       sigset_t mask;
                       sigfillset(&mask);
                       sigprocmask(SIG_SETMASK, &mask, NULL);

                       // Initialise our signal handler
                       init_signal_handler(catchable_signals);
#endif
                     }

                     // Start the backend worker
                     _backend_worker.run();

                     if (with_signal_handler)
                     {
#if defined(_WIN32)
        // ... ?
#else
                      // unblock all signals after spawning the thread
                      // note: we just assume that std::thread is implemented using posix threads
                      // or this won't have any effect
                      sigset_t mask;
                      sigemptyset(&mask);
                      sigprocmask(SIG_SETMASK, &mask, NULL);
#endif
                     }
                   });
  }

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop_backend_worker() noexcept { _backend_worker.stop(); }

  /**
   * @return true if backend worker has started
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD bool backend_worker_is_running() noexcept
  {
    return _backend_worker.is_running();
  }

#if !defined(QUILL_NO_EXCEPTIONS)
  /**
   * Set error handler
   * @param backend_worker_error_handler backend_worker_error_handler_t error handler
   * @throws exception if it is called after the thread has started
   */
  QUILL_ATTRIBUTE_COLD void set_backend_worker_error_handler(backend_worker_error_handler_t backend_worker_error_handler)
  {
    _backend_worker.set_error_handler(std::move(backend_worker_error_handler));
  }
#endif

private:
  Config _config;
  HandlerCollection _handler_collection;
  ThreadContextCollection _thread_context_collection{_config};
  LoggerCollection _logger_collection{_thread_context_collection, _handler_collection};
  BackendWorker _backend_worker{_config, _thread_context_collection, _handler_collection};
  std::once_flag _start_init_once_flag; /** flag to start the thread only once, in case start() is called multiple times */
  std::string _process_id = fmt::format_int(get_process_id()).str();
};
} // namespace detail
} // namespace quill