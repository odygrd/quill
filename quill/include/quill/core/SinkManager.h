/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/QuillError.h"
#include "quill/core/handlers/Handler.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace quill::detail {
    class SinkManager {
    public:
        SinkManager(SinkManager const &) = delete;

        SinkManager &operator=(SinkManager const &) = delete;

        /***/
        QUILL_API static SinkManager
        &

        instance() noexcept {
            static SinkManager instance;
            return instance;
        }

        /***/
        template<typename THandler, typename... Args>
        QUILL_NODISCARD QUILL_ATTRIBUTE_COLD

        std::shared_ptr<Handler> create_or_get_sink(std::string const &sink_name,
                                                    Args &&... args) {
            // The sinks are used by the backend thread, so after their creation we want to avoid mutating their member variables.

            std::lock_guard<std::mutex> const lock{_mutex};
            std::shared_ptr<Handler> sink;

            // First search if we have it and don't create it
            // We do this because creation will call fopen and we want to avoid that
            // Check if we already have this object
            if (auto const search = _sinks.find(sink_name); search != std::cend(_sinks)) {
                sink = search->second.lock();

                if (QUILL_UNLIKELY(!sink)) {
                    // recreate this handler
                    sink = std::make_shared<THandler>(std::forward<Args>(args)...);
                    search->second = sink;
                }
            } else {
                // if first time add it
                sink = std::make_shared<THandler>(std::forward<Args>(args)...);
                _sinks.emplace(sink_name, sink);
            }

            return sink;
        }

        /***/
        QUILL_NODISCARD QUILL_ATTRIBUTE_COLD

        std::shared_ptr<Handler> get_sink(std::string const &sink_name) const {
            // The sinks are used by the backend thread, so after their creation we want to avoid mutating their member variables.

            std::lock_guard<std::mutex> const lock{_mutex};

            if (auto const search = _sinks.find(sink_name); search != std::cend(_sinks)) {
                std::shared_ptr<Handler> sink = search->second.lock();

                if (QUILL_LIKELY(sink != nullptr)) {
                    return sink;
                }

                QUILL_THROW(QuillError{"Sink with name \"" + sink_name + "\" is being removed"});
            }

            QUILL_THROW(QuillError{"Handler with name \"" + sink_name + "\" does not exist"});
        }

        /***/
        void cleanup_unused_sinks() {
            // this needs to take a lock each time. The backend logging thread should be carefully call
            // it only when needed
            std::lock_guard<std::mutex> const lock{_mutex};

            for (auto it = std::begin(_sinks); it != std::end(_sinks);) {
                if (it->second.expired()) {
                    it = _sinks.erase(it);
                } else {
                    ++it;
                }
            }
        }

    private:
        SinkManager() = default;

        ~SinkManager() = default;

    private:
        std::unordered_map<std::string, std::weak_ptr<Handler>> _sinks;
        mutable std::mutex _mutex;
    };
} // namespace quill::detail