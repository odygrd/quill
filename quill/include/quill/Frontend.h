/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/frontend/LogMacros.h"

#include "quill/core/Attributes.h"
#include "quill/core/LoggerManager.h"
#include "quill/core/SinkManager.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/frontend/Logger.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>

namespace quill {
/**
 * Pre-allocates the thread-local data needed for the current thread.
 * Although optional, it is recommended to invoke this function during the thread initialisation
 * phase before the first log message.
 */
    QUILL_ATTRIBUTE_COLD inline void preallocate() {
        QUILL_MAYBE_UNUSED
        uint32_t const volatile x =
                detail::get_local_thread_context<QUILL_QUEUE_TYPE>()->spsc_queue<QUILL_QUEUE_TYPE>().capacity();
    }

    template<typename TSink, typename... Args>
    QUILL_NODISCARD QUILL_ATTRIBUTE_COLD

    std::shared_ptr<Handler> create_or_get_sink(std::string const &sink_name,
                                                Args &&... args) {
        return detail::SinkManager::instance().create_or_get_sink<TSink>(sink_name, std::forward<Args>(args)...);
    }

    QUILL_NODISCARD QUILL_ATTRIBUTE_COLD

    std::shared_ptr<Handler> get_sink(std::string const &sink_name) {
        return detail::SinkManager::instance().get_sink(sink_name);
    }

    QUILL_NODISCARD Logger
    *
    create_or_get_logger(std::string
    const& logger_name,
    std::shared_ptr<Handler> sink,
            ClockSourceType
    timestamp_clock_type,
    UserClock *timestamp_clock
    ) {
    return

    detail::LoggerManager::instance()

    .
    create_or_get_logger(
            logger_name, std::move(sink), timestamp_clock_type, timestamp_clock
    );
}

QUILL_NODISCARD Logger
*
create_or_get_logger(std::string
const& logger_name,
std::initializer_list<std::shared_ptr<Handler>> sinks,
        ClockSourceType
timestamp_clock_type,
UserClock *timestamp_clock
)
{
return

detail::LoggerManager::instance()

.
create_or_get_logger(
        logger_name, sinks, timestamp_clock_type, timestamp_clock
);
}

void remove_logger(Logger * logger) { detail::LoggerManager::instance().remove_logger(logger); }

QUILL_NODISCARD Logger
*
get_logger(std::string
const& logger_name)
{
return

detail::LoggerManager::instance()

.
get_logger(logger_name);
}

QUILL_NODISCARD std::unordered_map<std::string, Logger *>

get_all_loggers() {
    return detail::LoggerManager::instance().get_all_loggers();
}

} // namespace quill