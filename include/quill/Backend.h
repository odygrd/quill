/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/BackendManager.h"
#include "quill/backend/BackendOptions.h"
#include "quill/backend/SignalHandler.h"
#include "quill/core/Attributes.h"
#include "quill/core/MetricManager.h"
#include "quill/core/QuillError.h"

#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <mutex>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/** Version Info - When updating VersionMajor please also update the namespace in Attributes.h **/
inline constexpr uint32_t VersionMajor{12};
inline constexpr uint32_t VersionMinor{0};
inline constexpr uint32_t VersionPatch{0};
inline constexpr uint32_t Version{VersionMajor * 10000 + VersionMinor * 100 + VersionPatch};

class Backend
{
public:
  /**
   * Starts the backend thread.
   * @param options Backend options to configure the backend behavior.
   * @note Concurrent calls to start() and stop() from different threads are not supported.
   */
  QUILL_ATTRIBUTE_COLD static void start(BackendOptions const& options = BackendOptions{})
  {
    std::call_once(detail::BackendManager::instance().get_start_once_flag(),
                   [options]()
                   {
                     // Static-destruction ordering matters here because Backend::stop() runs via
                     // std::atexit. Any singleton the backend may touch during shutdown must be
                     // constructed before we register the atexit handler, otherwise it could be
                     // destroyed first and the backend could dereference freed state while draining
                     // queues.
                     //
                     // BackendManager construction already pins ThreadContextManager,
                     // SinkManager, and LoggerManager because BackendWorker stores references to
                     // them as members. SignalHandlerContext and MetricManager are not pinned that
                     // way, so construct them explicitly before atexit registration.
                     (void)detail::SignalHandlerContext::instance();
                     (void)detail::MetricManager::instance();

                     // Run the backend worker thread, we wait here until the thread enters the main loop
                     detail::BackendManager::instance().start_backend_thread(options);

                     // Set up an exit handler to call stop when the main application exits.
                     // always call stop on destruction to log everything. std::atexit seems to be
                     // working better with dll on windows compared to using ~LogManagerSingleton().
                     if (!detail::BackendManager::instance().is_atexit_registered())
                     {
                       detail::BackendManager::instance().set_atexit_registered();
                       std::atexit([]() { Backend::stop(); });
                     }
                   });
  }

  /**
   * Starts the backend thread and initialises a signal handler.
   *
   * @param backend_options Backend options to configure the backend behavior.
   * @param signal_handler_options SignalHandler options to configure the signal handler behavior.
   *
   * @note Enabling the built-in signal handler overrides the listed process signal handlers.
   * @note Concurrent calls to start() and stop() from different threads are not supported.
   *
   * @note When using the SignalHandler on Linux/MacOS, ensure that each spawned thread in your
   * application has performed one of the following actions:
   * i) Logged at least once.
   * or ii) Called Frontend::preallocate().
   * or iii) Blocked signals on that thread to prevent the signal handler from running on it.
   * This requirement is because the built-in signal handler utilizes the frontend queue state to
   * issue log statements and wait for flushing. The queue is constructed on its first use with
   * `new()`. Failing to meet any of the above criteria means the queue may be first-constructed
   * inside the signal handler, which is not signal-safe. Preallocating or logging once on those
   * threads avoids that first-use allocation, but the built-in handler should still be treated as a
   * best-effort crash-preservation facility rather than a general async-signal-safe logging API.
   *
   * @note On Windows, Backend::start() installs structured exception and console control handlers.
   *       CRT signal handlers are thread-specific and must be installed explicitly with
   *       init_signal_handler<TFrontendOptions>() on each frontend/user thread that needs them,
   *       not on the backend worker thread.
   */
  template <typename TFrontendOptions>
  QUILL_ATTRIBUTE_COLD static void start(BackendOptions const& backend_options,
                                         SignalHandlerOptions const& signal_handler_options)
  {
    std::call_once(
      detail::BackendManager::instance().get_start_once_flag(),
      [backend_options, signal_handler_options]()
      {
        // These flags are only read in the catch block; unused in no-exception builds.
        QUILL_MAYBE_UNUSED bool signal_handler_initialized{false};
#if defined(_WIN32)
        QUILL_MAYBE_UNUSED bool exception_handler_initialized{false};
#else
        sigset_t set, oldset;
        QUILL_MAYBE_UNUSED bool signal_mask_modified{false};
#endif
        QUILL_TRY
        {
          // See Backend::start(BackendOptions) for the shutdown-order rationale.
          // BackendManager construction already pins ThreadContextManager,
          // SinkManager, and LoggerManager through BackendWorker member references.
          (void)detail::MetricManager::instance();

#if defined(_WIN32)
          detail::init_exception_handler<TFrontendOptions>();
          exception_handler_initialized = true;
#else
          // We do not want the signal handler to run in the backend worker thread.
          // Block signals in the caller thread so the backend worker inherits that mask.
          sigfillset(&set);
          if (sigprocmask(SIG_SETMASK, &set, &oldset) != 0)
          {
            QUILL_THROW(QuillError{"Failed to block signals before starting the backend thread"});
          }
          signal_mask_modified = true;
          detail::init_signal_handler<TFrontendOptions>(signal_handler_options.catchable_signals);
          signal_handler_initialized = true;
#endif

          auto& signal_handler_context = detail::SignalHandlerContext::instance();
          signal_handler_context.logger_name = signal_handler_options.logger_name;
          signal_handler_context.excluded_logger_substrings = signal_handler_options.excluded_logger_substrings;
          signal_handler_context.signal_handler_timeout_seconds.store(signal_handler_options.timeout_seconds);

          // Run the backend worker thread, we wait here until the thread enters the main loop
          detail::BackendManager::instance().start_backend_thread(backend_options);

          // We need to update the signal handler with some backend thread details
          signal_handler_context.backend_thread_id.store(
            detail::BackendManager::instance().get_backend_thread_id());

#if defined(_WIN32)
        // nothing to do
#else
          // Unblock signals in the caller thread so subsequent threads do not inherit the blocked mask
          if (sigprocmask(SIG_SETMASK, &oldset, nullptr) != 0)
          {
            QUILL_THROW(
              QuillError{"Failed to restore the caller signal mask after backend startup"});
          }
          signal_mask_modified = false;
#endif

          // Set up an exit handler to call stop when the main application exits.
          // always call stop on destruction to log everything. std::atexit seems to be
          // working better with dll on windows compared to using ~LogManagerSingleton().
          if (!detail::BackendManager::instance().is_atexit_registered())
          {
            detail::BackendManager::instance().set_atexit_registered();
            std::atexit([]() { Backend::stop(); });
          }
        }
#if !defined(QUILL_NO_EXCEPTIONS)
        QUILL_CATCH(...)
        {
          if (signal_handler_initialized)
          {
            detail::restore_signal_handlers();
          }
  #if defined(_WIN32)
          if (exception_handler_initialized)
          {
            detail::deinit_exception_handler<TFrontendOptions>();
          }
  #else
          if (signal_mask_modified)
          {
            sigprocmask(SIG_SETMASK, &oldset, nullptr);
          }
  #endif
          throw;
        }
#endif
      });
  }

  /**
   * Stops the backend thread.
   *
   * @note On POSIX systems, when the built-in signal handler is enabled, this restores the
   * Quill-managed signals to their default dispositions. It does not restore any previously
   * installed user handlers.
   * @note Concurrent calls to start() and stop() from different threads are not supported.
   * @note This function must not be called from backend-thread callbacks because it joins the
   *       backend worker thread.
   * @note thread-safe
   */
  QUILL_ATTRIBUTE_COLD static void stop()
  {
    uint32_t const backend_thread_id = detail::BackendManager::instance().get_backend_thread_id();
    if (QUILL_UNLIKELY((backend_thread_id != 0) && (backend_thread_id == detail::get_thread_id())))
    {
      QUILL_THROW(QuillError{"Backend::stop() cannot be called from the backend worker thread"});
    }

    detail::SignalHandlerContext::instance().backend_thread_id.store(0);
    detail::BackendManager::instance().stop_backend_thread();
#if defined(_WIN32)
    if (auto const fn = detail::SignalHandlerContext::instance().exception_handler_deinit_callback)
    {
      fn();
    }
#endif
    detail::deinit_signal_handler();
  }

  /**
   * Notifies the backend thread to wake up.
   * It is possible to use a long backend sleep_duration and then notify the backend to wake up
   * from any frontend thread.
   *
   * @note thread-safe
   */
  static void notify() noexcept { detail::BackendManager::instance().notify_backend_thread(); }

  /**
   * Checks if the backend is currently running.
   * @return True if the backend is running, false otherwise.
   */
  QUILL_NODISCARD static bool is_running() noexcept
  {
    return detail::BackendManager::instance().is_backend_thread_running();
  }

  /**
   * Retrieves the ID of the backend thread.
   * @return The ID of the backend thread.
   */
  QUILL_NODISCARD static uint32_t get_thread_id() noexcept
  {
    return detail::BackendManager::instance().get_backend_thread_id();
  }

  /**
   * Converts an rdtsc value to epoch time.
   * This function uses the same clock as the backend and can be called from any frontend thread.
   * It is useful when using a logger with ClockSourceType::Tsc and you want to obtain a timestamp
   * synchronized with the log files generated by the backend.
   *
   * Alternatively you can use the Clock class from backend/Clock.h
   * @param rdtsc_value The RDTSC value to convert.
   * @return The epoch time corresponding to the RDTSC value.
   * @throws QuillError if the backend TSC configuration is invalid.
   */
  QUILL_NODISCARD static uint64_t convert_rdtsc_to_epoch_time(uint64_t rdtsc_value)
  {
    return detail::BackendManager::instance().convert_rdtsc_to_epoch_time(rdtsc_value);
  }

  /**
   * This feature is designed for advanced users who need to run the backend worker on their own thread,
   * providing more flexibility at the cost of complexity and potential pitfalls.
   *
   * This approach is generally not recommended due to the potential for inefficiency and complexity
   * in managing the backend worker outside the provided mechanisms.
   *
   * Important notes:
   *   - Do not use this to run the library in a single threaded application. This will lead to inefficiencies.
   *     The design of this logging library assumes that the backend worker operates in a separate thread from the frontend threads that issue log statements.
   *   - The thread running the `ManualBackendWorker` can log, but it must not use backend-waiting
   *     flush paths from that same thread. See `ManualBackendWorker` for the manual-backend
   *     threading contract.
   *   - The `ManualBackendWorker` should only be used by a single thread. It is not designed to handle
   *     multiple threads calling `poll()` simultaneously.
   *   - You must call `ManualBackendWorker::shutdown()` explicitly from the same thread that called
   *     `init()` before that thread exits. Do not rely on the `ManualBackendWorker` destructor for
   *     shutdown ordering.
   *   - The built-in signal handler is not set up with `ManualBackendWorker`. If signal handling is
   *     required, you must manually set up the signal handler and block signals from reaching the `ManualBackendWorker` thread.
   *     See the `start<FrontendOptions>(BackendOptions, SignalHandlerOptions)` implementation for guidance on how to do this.
   *   - The following options are not supported when using `ManualBackendWorker`: `cpu_affinity`,
   *     `thread_name`, `sleep_duration`, and `enable_yield_when_idle`.
   *   - Avoid performing very heavy tasks in your custom thread. Significant delays in calling `poll()`
   *     can lead to the SPSC queues of the frontend threads becoming full. When this happens, the
   *     frontend threads may need to allocate additional memory on the hot path.
   *
   * @code
   * std::thread backend_worker([]()
   *   {
   *     quill::ManualBackendWorker* manual_backend_worker = quill::Backend::acquire_manual_backend_worker();
   *
   *     quill::BackendOptions backend_options;
   *     manual_backend_worker->init(backend_options);
   *
   *     while (running)
   *     {
   *       manual_backend_worker->poll();
   *     }
   *
   *     manual_backend_worker->shutdown();
   *   });
   * @endcode
   */
  QUILL_ATTRIBUTE_COLD static ManualBackendWorker* acquire_manual_backend_worker()
  {
    ManualBackendWorker* manual_backend_worker{nullptr};

    // If a caller forgets to perform explicit ManualBackendWorker::shutdown(), the
    // ManualBackendWorker destructor can still drain queued metric events during static
    // destruction. Construct MetricManager before BackendManager so MetricMetadata stays alive
    // for that fallback drain path.
    (void)detail::MetricManager::instance();

    std::call_once(
      detail::BackendManager::instance().get_start_once_flag(), [&manual_backend_worker]() mutable
      { manual_backend_worker = detail::BackendManager::instance().get_manual_backend_worker(); });

    if (!manual_backend_worker)
    {
      QUILL_THROW(
        QuillError{"acquire_manual_backend_worker() can only be called once per process. "
                   "Additionally, it should not be "
                   "called when start() has already been invoked"});
    }

    return manual_backend_worker;
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
