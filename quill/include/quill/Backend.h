/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/BackendManager.h"
#include "quill/core/Config.h"
#include "quill/core/Attributes.h"

namespace quill {
    QUILL_ATTRIBUTE_COLD void start_backend_thread(Config const &cfg, bool with_signal_handler,
                                                   std::initializer_list<int> const &catchable_signals) {
        detail::BackendManager::instance().start_backend_thread(cfg, with_signal_handler, catchable_signals);
    }

    QUILL_ATTRIBUTE_COLD void stop_backend_thread() noexcept {
        detail::BackendManager::instance().stop_backend_thread();
    }

    QUILL_ATTRIBUTE_COLD void notify_backend_thread() noexcept {
        detail::BackendManager::instance().notify_backend_thread();
    }

    QUILL_NODISCARD QUILL_ATTRIBUTE_COLD

    bool is_backend_thread_running() noexcept {
        return detail::BackendManager::instance().is_backend_thread_running();
    }

    QUILL_NODISCARD uint32_t

    get_backend_thread_id() noexcept {
        return detail::BackendManager::instance().get_backend_thread_id();
    }

    QUILL_NODISCARD uint64_t
    convert_rdtsc_to_epoch_time(uint64_t
    rdtsc_value) noexcept {
    return

    detail::BackendManager::instance()

    .
    convert_rdtsc_to_epoch_time(rdtsc_value);
}
} // namespace quill