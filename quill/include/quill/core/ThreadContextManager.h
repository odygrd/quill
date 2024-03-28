/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/core/Common.h"
#include "quill/core/Os.h"
#include "quill/core/TransitEventBuffer.h"
#include "quill/core/UnboundedSPSCQueue.h"
#include "quill/core/Attributes.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <variant>
#include <vector>

namespace quill
{
namespace detail
{

    class ThreadContext {
    public:
        /***/
        ThreadContext(QueueType queue_type, uint32_t initial_spsc_queue_capacity,
                      uint32_t initial_transit_buffer_capacity, bool use_huge_pages)
                : _transit_event_buffer(initial_transit_buffer_capacity) {
            if ((queue_type == QueueType::UnboundedBlocking) ||
                (queue_type == QueueType::UnboundedNoMaxLimit) || (queue_type == QueueType::UnboundedDropping)) {
                _spsc_queue.emplace<UnboundedSPSCQueue>(initial_spsc_queue_capacity, use_huge_pages);
            } else {
                _spsc_queue.emplace<BoundedSPSCQueue>(initial_spsc_queue_capacity, use_huge_pages);
            }
        }

        /***/
        ThreadContext(ThreadContext const &) = delete;

        ThreadContext &operator=(ThreadContext const &) = delete;

        /***/
        QUILL_NODISCARD_ALWAYS_INLINE_HOT UnboundedTransitEventBuffer
        &

        transit_event_buffer() noexcept {
            return _transit_event_buffer;
        }

        /***/
        template<QueueType queue_type>
        QUILL_NODISCARD_ALWAYS_INLINE_HOT std::conditional_t<
                (queue_type == QueueType::UnboundedBlocking) || (queue_type == QueueType::UnboundedNoMaxLimit) ||
                (queue_type == QueueType::UnboundedDropping),
                UnboundedSPSCQueue, BoundedSPSCQueue>
        &

        spsc_queue() noexcept {
            if constexpr ((queue_type == QueueType::UnboundedBlocking) ||
                          (queue_type == QueueType::UnboundedNoMaxLimit) ||
                          (queue_type == QueueType::UnboundedDropping)) {
                return std::get<UnboundedSPSCQueue>(_spsc_queue);
            } else {
                return std::get<BoundedSPSCQueue>(_spsc_queue);
            }
        }

        /***/
        template<QueueType queue_type>
        QUILL_NODISCARD_ALWAYS_INLINE_HOT std::conditional_t<
                (queue_type == QueueType::UnboundedBlocking) || (queue_type == QueueType::UnboundedNoMaxLimit) ||
                (queue_type == QueueType::UnboundedDropping),
                UnboundedSPSCQueue, BoundedSPSCQueue>
        const&

        spsc_queue() const noexcept {
            if constexpr ((queue_type == QueueType::UnboundedBlocking) ||
                          (queue_type == QueueType::UnboundedNoMaxLimit) ||
                          (queue_type == QueueType::UnboundedDropping)) {
                return std::get<UnboundedSPSCQueue>(_spsc_queue);
            } else
    {
        return std::get<BoundedSPSCQueue>(_spsc_queue);
    }
        }

        /***/
        QUILL_NODISCARD_ALWAYS_INLINE_HOT std::variant<std::monostate, UnboundedSPSCQueue, BoundedSPSCQueue>
        const&

        spsc_queue_variant() const noexcept {
            return _spsc_queue;
        }

        /***/
        QUILL_NODISCARD_ALWAYS_INLINE_HOT std::variant<std::monostate, UnboundedSPSCQueue, BoundedSPSCQueue>
        &

        spsc_queue_variant() noexcept {
            return _spsc_queue;
        }

        /***/
        QUILL_NODISCARD char const *thread_id() const noexcept { return _thread_id.data(); }

        /***/
        QUILL_NODISCARD char const *thread_name() const noexcept { return _thread_name.data(); }

        /***/
        void mark_invalid() noexcept { _valid.store(false, std::memory_order_relaxed); }

        /***/
        QUILL_NODISCARD bool is_valid_context() const noexcept { return _valid.load(std::memory_order_relaxed); }

        /***/
        void increment_failure_counter() noexcept {
            _failure_counter.fetch_add(1, std::memory_order_relaxed);
        }

        /***/
        QUILL_NODISCARD QUILL_ATTRIBUTE_HOT

        size_t get_and_reset_failure_counter() noexcept {
            if (QUILL_LIKELY(_failure_counter.load(std::memory_order_relaxed) == 0))
    {
        return 0;
    }
            return _failure_counter.exchange(0, std::memory_order_relaxed);
        }

    private:
        std::variant<std::monostate, UnboundedSPSCQueue, BoundedSPSCQueue> _spsc_queue; /** queue for this thread, events are pushed here */
        UnboundedTransitEventBuffer _transit_event_buffer;        /** backend thread buffer */
        std::string _thread_id = std::to_string(get_thread_id()); /**< cache this thread pid */
        std::string _thread_name = get_thread_name();             /**< cache this thread name */
        std::atomic<bool> _valid{
                true}; /**< is this context valid, set by the caller, read by the backend worker thread */
        alignas(CACHE_LINE_ALIGNED) std::atomic<size_t> _failure_counter{0};
    };

    class ThreadContextManager {
public:
        /***/
        QUILL_API static ThreadContextManager
        &

        instance() noexcept {
            static ThreadContextManager instance;
            return instance;
        }

        /***/
        ThreadContextManager(ThreadContextManager const &) = delete;

        ThreadContextManager &operator=(ThreadContextManager const &) = delete;

        /***/
        template<typename TCallback>
        void for_each_thread_context(TCallback cb) {
            std::lock_guard<std::mutex> const lock{_mutex};

            for (auto const &elem: _thread_contexts) {
                cb(elem.get());
            }
  }

        /***/
  void register_thread_context(std::shared_ptr<ThreadContext> const& thread_context)
  {
    _mutex.lock();
    _thread_contexts.push_back(thread_context);
    _mutex.unlock();
      _new_thread_context_flag.store(true, std::memory_order_release);
  }

        /***/
        void sub_invalid_thread_context() noexcept {
            _invalid_thread_context_count.fetch_sub(1, std::memory_order_relaxed);
        }

        /***/
        void add_invalid_thread_context() noexcept {
            _invalid_thread_context_count.fetch_add(1, std::memory_order_relaxed);
        }

        /***/
        QUILL_NODISCARD QUILL_ATTRIBUTE_HOT

        bool has_invalid_thread_context() const noexcept
  {
      // Here we do relaxed because if the value is not zero we will look inside ThreadContext invalid
      // flag that is also a relaxed atomic, and then we will look into the SPSC queue size that is
      // also atomic Even if we don't read everything in order we will check again in the next circle
      return _invalid_thread_context_count.load(std::memory_order_relaxed) != 0;
  }

        /***/
        QUILL_NODISCARD QUILL_ATTRIBUTE_HOT

        bool new_thread_context_flag() noexcept
  {
    // Again relaxed memory model as in case it is false we will acquire the lock
      if (_new_thread_context_flag.load(std::memory_order_relaxed))
    {
      // if the variable was updated to true, set it to false,
      // There should not be any race condition here as this is the only place _changed is set to
      // false, and we will return true anyway
        _new_thread_context_flag.store(false, std::memory_order_relaxed);
      return true;
    }
    return false;
  }

        /***/
        void remove_shared_invalidated_thread_context(ThreadContext const *thread_context)
  {
      std::lock_guard<std::mutex> const lock{_mutex};

    auto const thread_context_it = std::find_if(_thread_contexts.begin(), _thread_contexts.end(),
                                                [thread_context](std::shared_ptr<ThreadContext> const &elem) {
                                                    return elem.get() == thread_context;
                                                });

    assert(thread_context_it != _thread_contexts.end() &&
           "Attempting to remove_file a non existent thread context");

      assert(!thread_context_it->get()->is_valid_context() &&
           "Attempting to remove_file a valid thread context");

    assert(thread_context_it->get()->spsc_queue<QUILL_QUEUE_TYPE>().empty() &&
           "Attempting to remove_file a thread context with a non empty queue");

    _thread_contexts.erase(thread_context_it);

    // we don't set changed here as this is called only by the backend thread and it updates
    // the thread_contexts_cache itself after this function
  }

        /***/
        QUILL_NODISCARD uint32_t

        initial_spsc_queue_capacity() const noexcept {
            return _initial_spsc_queue_capacity;
        }

        /***/
        QUILL_NODISCARD uint32_t

        initial_transit_buffer_capacity() const noexcept {
            return _initial_transit_buffer_capacity;
        }

        /***/
        QUILL_NODISCARD bool huge_pages_frontend_enabled() const noexcept {
            return _huge_pages_frontend_enabled;
  }

private:
        ThreadContextManager() = default;

        ~ThreadContextManager() = default;

    private:
  std::mutex _mutex; /**< Protect access when register contexts or removing contexts */
  std::vector<std::shared_ptr<ThreadContext>> _thread_contexts; /**< The registered contexts */

        uint32_t _initial_spsc_queue_capacity = 131'072;
        uint32_t _initial_transit_buffer_capacity = 64;
        bool _huge_pages_frontend_enabled = false;

        alignas(CACHE_LINE_ALIGNED) std::atomic<bool> _new_thread_context_flag{false};
        alignas(CACHE_LINE_ALIGNED) std::atomic<uint8_t> _invalid_thread_context_count{0};
    };

    template<QueueType queue_type>
    class ScopedThreadContext {
    public:
        /***/
        ScopedThreadContext(uint32_t spsc_queue_capacity, uint32_t initial_transit_buffer_capacity, bool huge_pages)
                : _thread_context(std::make_shared<ThreadContext>(
                queue_type, spsc_queue_capacity, initial_transit_buffer_capacity, huge_pages)) {
            ThreadContextManager::instance().register_thread_context(_thread_context);
        }

        /***/
        ScopedThreadContext(ScopedThreadContext const &) = delete;

        ScopedThreadContext &operator=(ScopedThreadContext const &) = delete;

        /***/
        ~ScopedThreadContext() noexcept {
            // This destructor will get called when the thread that created this wrapper stops
            // we will only invalidate the thread context
            // The backend thread will empty an invalidated ThreadContext and then remove_file it from
            // the ThreadContextCollection
            // There is only exception for the thread who owns the ThreadContextCollection the
            // main thread. The thread context of the main thread can get deleted before getting invalidated
            _thread_context->mark_invalid();

            // Notify the backend thread that one context has been removed
            ThreadContextManager::instance().add_invalid_thread_context();
        }

        /***/
        QUILL_NODISCARD_ALWAYS_INLINE_HOT ThreadContext
        *

        get_thread_context() const noexcept {
            assert(_thread_context && "_thread_context can not be null");
            return _thread_context.get();
        }

    private:
  /**<
   * This could be unique_ptr but the thread context of main thread that owns
   * ThreadContextCollection can be destructed last even after the logger singleton destruction
   * so we use shared_ptr */
  std::shared_ptr<ThreadContext> _thread_context;
    };

/***/
    template<QueueType queue_type>
    QUILL_NODISCARD_ALWAYS_INLINE_HOT extern ThreadContext
    *

    get_local_thread_context() noexcept {
        thread_local ScopedThreadContext<queue_type> thread_context_wrapper{
                ThreadContextManager::instance().initial_spsc_queue_capacity(),
                ThreadContextManager::instance().initial_transit_buffer_capacity(),
                ThreadContextManager::instance().huge_pages_frontend_enabled()};

        return thread_context_wrapper.get_thread_context();
    }
} // namespace detail
} // namespace quill
