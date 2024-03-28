/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/backend/BackendWorker.h"

#include "quill/core/Config.h"
#include "quill/core/EncodeDecode.h"
#include "quill/core/UserClock.h"
#include "quill/detail/SignalHandler.h" // for init_signal_handler

#include <cassert>
#include <cstdlib>
#include <mutex> // for call_once, once_flag
#include <optional>

namespace quill::detail {
/**
 * Provides access to common collection class that are used by both the frontend and the backend
 * components of the logging system
 * There should only be only active active instance of this class which is achieved by the
 * LogSystemManagerSingleton
 */
    class BackendManager {
    public:
        /**
         * Access to singleton instance
         * @return a reference to the singleton
         */
        QUILL_API static BackendManager
        &

        instance() noexcept {
            static BackendManager instance;
            return instance;
        }

        /**
         * Deleted
         */
        BackendManager(BackendManager const &) = delete;

        BackendManager &operator=(BackendManager const &) = delete;

        QUILL_ATTRIBUTE_COLD void start_backend_thread(Config const &cfg, bool with_signal_handler,
                                                       std::initializer_list<int> const &catchable_signals) {
            // protect init to be called only once
            std::call_once(_start_init_once_flag,
                           [this, cfg, with_signal_handler, catchable_signals]() {
                               _start_backend_thread(cfg, with_signal_handler, catchable_signals);

                               // Setup a handler to call stop when the main application exits.
                               // always call stop on destruction to log everything. std::atexit seems to be
                               // working better with dll on windows compared to using ~LogManagerSingleton().
                               std::atexit([]() { BackendManager::instance().stop_backend_thread(); });
                           });
        }

        /**
         * Stops the backend worker thread
         */
        QUILL_ATTRIBUTE_COLD void stop_backend_thread() noexcept { _backend_worker.stop(); }

        /**
         * @return The current process id
         */
        QUILL_NODISCARD uint32_t

        get_backend_thread_id() const noexcept {
            return _backend_worker.thread_id();
        }

        /**
         * Wakes up the backend worker thread
         */
        QUILL_ATTRIBUTE_COLD void notify_backend_thread() noexcept { _backend_worker.wake_up(); }

        /**
         * @return true if backend worker has started
         */
        QUILL_NODISCARD QUILL_ATTRIBUTE_COLD

        bool is_backend_thread_running() const noexcept {
            return _backend_worker.is_running();
        }

        QUILL_NODISCARD uint64_t
        convert_rdtsc_to_epoch_time(uint64_t
        rdtsc_value) const noexcept
        {
            return _backend_worker.time_since_epoch(rdtsc_value);
        }

    private:
        /**
         * Constructor
         */
        BackendManager() = default;

        ~BackendManager() = default;

        /**
         * Starts the backend worker thread.
         * This should only be called by the LogSystemManagerSingleton and never directly from here
         */
        QUILL_ATTRIBUTE_COLD void _start_backend_thread(Config const &cfg, bool with_signal_handler,
                                                        std::initializer_list<int> const &catchable_signals) {
            if (with_signal_handler) {
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
            _backend_worker.run(cfg);

            if (with_signal_handler) {
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
        }

    private:
        BackendWorker _backend_worker;
        std::once_flag _start_init_once_flag;
    };
} // namespace quill::detail
