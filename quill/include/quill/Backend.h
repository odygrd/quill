/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/BackendManager.h"
#include "quill/backend/BackendOptions.h"
#include "quill/backend/SignalHandler.h"
#include "quill/core/Attributes.h"

#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <mutex>

/** Version Info **/
constexpr uint32_t VersionMajor{4};
constexpr uint32_t VersionMinor{2};
constexpr uint32_t VersionPatch{1};
constexpr uint32_t Version{VersionMajor * 10000 + VersionMinor * 100 + VersionPatch};

namespace quill
{
class Backend
{
public:
  /**
   * Starts the backend thread.
   * @param options Backend options to configure the backend behavior.
   */
  QUILL_ATTRIBUTE_COLD static void start(BackendOptions const& options = BackendOptions{})
  {
    std::call_once(detail::BackendManager::instance().get_start_once_flag(),
                   [options]()
                   {
                     // Run the backend worker thread, we wait here until the thread enters the main loop
                     detail::BackendManager::instance().start_backend_thread(options);

                     // Setup an exit handler to call stop when the main application exits.
                     // always call stop on destruction to log everything. std::atexit seems to be
                     // working better with dll on windows compared to using ~LogManagerSingleton().
                     std::atexit([]() { detail::BackendManager::instance().stop_backend_thread(); });
                   });
  }

  /**
   * Starts the backend thread and initialises a signal handler
   *
   * @param options Backend options to configure the backend behavior.
   * @param catchable_signals List of signals that the backend should catch if with_signal_handler
   * is enabled.
   * @param signal_handler_timeout_seconds This variable defines the timeout duration in seconds for
   * the signal handler alarm. It is only available on Linux, as Windows does not support the alarm
   * function. The signal handler sets up an alarm to ensure that the process will terminate if it
   * does not complete within the specified time frame. This is particularly useful to prevent the
   * process from hanging indefinitely in case the signal handler encounters an issue.
   *
   * @note When using the SignalHandler on Linux/MacOS, ensure that each spawned thread in your
   * application has performed one of the following actions:
   * i) Logged at least once.
   * ii) Called Frontend::preallocate().
   * iii) Blocked signals on that thread to prevent the signal handler from running on it.
   * This requirement is because the built-in signal handler utilizes a lock-free queue to issue log
   * statements and await the log flushing. The queue is constructed on its first use with `new()`.
   * Failing to meet any of the above criteria means the queue was never used, and it will be
   * constructed inside the signal handler. The `new` operation is not an async signal-safe function
   * and may lead to potential issues. However, when the queue is already created, no `new` call is
   * made, and the remaining functions invoked internally by the built-in signal handler are async
   * safe.
   */
  template <typename TFrontendOptions>
  QUILL_ATTRIBUTE_COLD static void start_with_signal_handler(
    BackendOptions const& options = BackendOptions{},
    QUILL_MAYBE_UNUSED std::initializer_list<int> const& catchable_signals =
      std::initializer_list<int>{SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV},
    uint32_t signal_handler_timeout_seconds = 20u)
  {
    std::call_once(detail::BackendManager::instance().get_start_once_flag(),
                   [options, catchable_signals, signal_handler_timeout_seconds]()
                   {
#if defined(_WIN32)
                     (void)catchable_signals;
                     detail::init_exception_handler<TFrontendOptions>();
#else
        // We do not want signal handler to run in the backend worker thread
        // Block signals in the main thread so when we spawn the backend worker thread it inherits
        // the master
        sigset_t set, oldset;
        sigfillset(&set);
        sigprocmask(SIG_SETMASK, &set, &oldset);
        detail::init_signal_handler<TFrontendOptions>(catchable_signals);
#endif

                     // Run the backend worker thread, we wait here until the thread enters the main loop
                     detail::BackendManager::instance().start_backend_thread(options);

                     detail::SignalHandlerContext::instance().signal_handler_timeout_seconds.store(
                       signal_handler_timeout_seconds);

                     // We need to update the signal handler with some backend thread details
                     detail::SignalHandlerContext::instance().backend_thread_id.store(
                       detail::BackendManager::instance().get_backend_thread_id());

#if defined(_WIN32)
      // nothing to do
#else
        // Unblock signals in the main thread so subsequent threads do not inherit the blocked mask
        sigprocmask(SIG_SETMASK, &oldset, nullptr);
#endif

                     // Set up an exit handler to call stop when the main application exits.
                     // always call stop on destruction to log everything. std::atexit seems to be
                     // working better with dll on windows compared to using ~LogManagerSingleton().
                     std::atexit([]() { detail::BackendManager::instance().stop_backend_thread(); });
                   });
  }

  /**
   * Stops the backend thread.
   * @note thread-safe
   */
  QUILL_ATTRIBUTE_COLD static void stop() noexcept
  {
    detail::BackendManager::instance().stop_backend_thread();
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
   */
  QUILL_NODISCARD static uint64_t convert_rdtsc_to_epoch_time(uint64_t rdtsc_value) noexcept
  {
    return detail::BackendManager::instance().convert_rdtsc_to_epoch_time(rdtsc_value);
  }
};
} // namespace quill