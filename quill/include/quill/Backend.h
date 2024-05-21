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
constexpr uint32_t VersionMinor{0};
constexpr uint32_t VersionPatch{0};
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
   * @param options Backend options to configure the backend behavior.
   * @param catchable_signals List of signals that the backend should catch if with_signal_handler is enabled.
   */
  template <typename TFrontendOptions>
  QUILL_ATTRIBUTE_COLD static void start_with_signal_handler(
    BackendOptions const& options = BackendOptions{},
    QUILL_MAYBE_UNUSED std::initializer_list<int> const& catchable_signals = std::initializer_list<int>{
      SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV})
  {
    std::call_once(detail::BackendManager::instance().get_start_once_flag(),
                   [options, catchable_signals]()
                   {
#if defined(_WIN32)
                     (void)catchable_signals;
                     detail::init_exception_handler<TFrontendOptions>();
#else
        // block all signals before spawning the backend worker thread
        // note: we just assume that std::thread is implemented using posix threads
        // or this won't have any effect
        sigset_t fill_mask;
        sigfillset(&fill_mask);
        sigprocmask(SIG_SETMASK, &fill_mask, nullptr);
        detail::init_signal_handler<TFrontendOptions>(catchable_signals);
#endif

                     // Run the backend worker thread, we wait here until the thread enters the main loop
                     detail::BackendManager::instance().start_backend_thread(options);

                     // We need to update the signal handler with some backend thread details
                     detail::SignalHandlerContext::instance().backend_thread_id.store(
                       detail::BackendManager::instance().get_backend_thread_id());

#if defined(_WIN32)
      // nothing to do
#else
                     // unblock all signals after spawning the backend thread
                     // note: we just assume that std::thread is implemented using posix threads
                     // or this won't have any effect
                     sigset_t empty_mask;
                     sigemptyset(&empty_mask);
                     sigprocmask(SIG_SETMASK, &empty_mask, nullptr);
#endif

                     // Setup an exit handler to call stop when the main application exits.
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
  QUILL_ATTRIBUTE_COLD static void notify() noexcept
  {
    detail::BackendManager::instance().notify_backend_thread();
  }

  /**
   * Checks if the backend is currently running.
   * @return True if the backend is running, false otherwise.
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD static bool is_running() noexcept
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