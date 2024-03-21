/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/Config.h"
#include "quill/Logger.h"
#include "quill/clock/TimestampClock.h"
#include "quill/detail/HandlerCollection.h"
#include "quill/detail/LoggerCollection.h"
#include "quill/detail/Serialize.h"
#include "quill/detail/SignalHandler.h" // for init_signal_handler
#include "quill/detail/ThreadContextCollection.h"
#include "quill/detail/backend/BackendWorker.h"
#include <cassert>
#include <cstdlib>
#include <mutex> // for call_once, once_flag
#include <optional>

namespace quill::detail
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
  LogManager() { _configure(); }

  /**
   * Deleted
   */
  LogManager(LogManager const&) = delete;
  LogManager& operator=(LogManager const&) = delete;

  QUILL_ATTRIBUTE_COLD void configure(Config const& cfg) noexcept
  {
    _config = cfg;
    _configure();
  }

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
  QUILL_NODISCARD uint32_t backend_worker_thread_id() const noexcept
  {
    return _backend_worker.thread_id();
  }

  /***/
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                        std::optional<TimestampClockType> timestamp_clock_type,
                                        std::optional<TimestampClock*> timestamp_clock)
  {
    return _logger_collection.create_logger(
      logger_name, timestamp_clock_type.has_value() ? *timestamp_clock_type : _config.default_timestamp_clock_type,
      timestamp_clock.has_value() ? *timestamp_clock : _config.default_custom_timestamp_clock);
  }

  /***/
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, std::shared_ptr<Handler> handler,
                                        std::optional<TimestampClockType> timestamp_clock_type,
                                        std::optional<TimestampClock*> timestamp_clock)
  {
    return _logger_collection.create_logger(
      logger_name, std::move(handler),
      timestamp_clock_type.has_value() ? *timestamp_clock_type : _config.default_timestamp_clock_type,
      timestamp_clock.has_value() ? *timestamp_clock : _config.default_custom_timestamp_clock);
  }

  /***/
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                        std::initializer_list<std::shared_ptr<Handler>> handlers,
                                        std::optional<TimestampClockType> timestamp_clock_type,
                                        std::optional<TimestampClock*> timestamp_clock)
  {
    return _logger_collection.create_logger(
      logger_name, std::move(handlers),
      timestamp_clock_type.has_value() ? *timestamp_clock_type : _config.default_timestamp_clock_type,
      timestamp_clock.has_value() ? *timestamp_clock : _config.default_custom_timestamp_clock);
  }

  /***/
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                        std::vector<std::shared_ptr<Handler>> handlers,
                                        std::optional<TimestampClockType> timestamp_clock_type,
                                        std::optional<TimestampClock*> timestamp_clock)
  {
    return _logger_collection.create_logger(
      logger_name, std::move(handlers),
      timestamp_clock_type.has_value() ? *timestamp_clock_type : _config.default_timestamp_clock_type,
      timestamp_clock.has_value() ? *timestamp_clock : _config.default_custom_timestamp_clock);
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

    // get the root logger - this is needed for the logger_details struct, in order to figure out
    // the clock type later on the backend thread
    Logger* default_logger = logger_collection().get_logger(nullptr);
    LoggerDetails const* logger_details = &default_logger->_logger_details;

    // Take and store the timestamp here as it is more accurate
    uint64_t const timestamp = (logger_details->timestamp_clock_type() == TimestampClockType::Tsc)
      ? detail::rdtsc()
      : (logger_details->timestamp_clock_type() == TimestampClockType::System)
      ? static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count())
      : default_logger->_custom_timestamp_clock->now();

    // Create an atomic variable
    std::atomic<bool> backend_thread_flushed{false};

    // we need to write an event to the queue passing this atomic variable
    ThreadContext* const thread_context =
      _thread_context_collection.local_thread_context<QUILL_QUEUE_TYPE>();

    auto& spsc_queue = thread_context->spsc_queue<QUILL_QUEUE_TYPE>();
    size_t constexpr total_size = alignof(uint64_t) + sizeof(uint64_t) + (sizeof(uintptr_t) * 4);

    std::byte* write_buffer;
    while ((write_buffer = spsc_queue.prepare_write(static_cast<uint32_t>(total_size))) == nullptr)
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    }

    std::byte const* const write_begin = write_buffer;

    write_buffer = detail::align_pointer<alignof(uint64_t), std::byte>(write_buffer);

    std::memcpy(write_buffer, &timestamp, sizeof(timestamp));
    write_buffer += sizeof(timestamp);

    static constexpr MacroMetadata macro_metadata{
      "", "", "", nullptr, LogLevel::Critical, MacroMetadata::Event::Flush, false, false};
    MacroMetadata const* macro_metadata_ptr = &macro_metadata;

    std::memcpy(write_buffer, &macro_metadata_ptr, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    std::memcpy(write_buffer, &logger_details, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    // Increment once more we are not writing any format_to function for flush
    write_buffer += sizeof(uintptr_t);

    // encode the pointer to atomic bool
    std::atomic<bool>* flush_ptr = std::addressof(backend_thread_flushed);
    std::memcpy(write_buffer, &flush_ptr, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    assert((write_buffer >= write_begin) &&
           "write_buffer should be greater or equal to write_begin");

    thread_context->spsc_queue<QUILL_QUEUE_TYPE>().finish_write(static_cast<uint32_t>(write_buffer - write_begin));
    thread_context->spsc_queue<QUILL_QUEUE_TYPE>().commit_write();

    // The caller thread keeps checking the flag until the backend thread flushes
    do
    {
      // wait
      std::this_thread::sleep_for(std::chrono::nanoseconds{100});
    } while (!backend_thread_flushed.load());
  }

  /**
   * Starts the backend worker thread.
   * This should only be called by the LogManagerSingleton and never directly from here
   */
  QUILL_ATTRIBUTE_COLD void start_backend_worker(bool with_signal_handler,
                                                 std::initializer_list<int> const& catchable_signals)
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
  }

  /**
   * Stops the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void stop_backend_worker() noexcept { _backend_worker.stop(); }

  /**
   * Wakes up the backend worker thread
   */
  QUILL_ATTRIBUTE_COLD void wake_up_backend_worker() noexcept { _backend_worker.wake_up(); }

  /**
   * @return true if backend worker has started
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD bool backend_worker_is_running() const noexcept
  {
    return _backend_worker.is_running();
  }

  QUILL_NODISCARD uint64_t time_since_epoch(uint64_t rdtsc_value) const noexcept
  {
    return _backend_worker.time_since_epoch(rdtsc_value);
  }

private:
  void _configure()
  {
    // re-create the root logger with the given config
    _logger_collection.create_root_logger();
  }

private:
  Config _config;
  HandlerCollection _handler_collection;
  ThreadContextCollection _thread_context_collection{_config};
  LoggerCollection _logger_collection{_config, _thread_context_collection, _handler_collection};
  BackendWorker _backend_worker{_config, _thread_context_collection, _handler_collection, _logger_collection};
};

/**
 * A wrapper class around LogManager to make LogManager act as a singleton.
 * In fact LogManager is always a singleton as every access is provided via this class but this
 * gives us the possibility have multiple unit tests for LogManager as it would be harder to test
 * a singleton class
 */
class LogManagerSingleton
{
public:
  LogManagerSingleton(LogManagerSingleton const&) = delete;
  LogManagerSingleton& operator=(LogManagerSingleton const&) = delete;

  /**
   * Access to singleton instance
   * @return a reference to the singleton
   */
  QUILL_API static LogManagerSingleton& instance() noexcept
  {
    static LogManagerSingleton instance;
    return instance;
  }

  /**
   * Access to LogManager
   * @return a reference to the log manager
   */
  LogManager& log_manager() noexcept { return _log_manager; }

  QUILL_ATTRIBUTE_COLD void start_backend_worker(bool with_signal_handler,
                                                 std::initializer_list<int> const& catchable_signals)
  {
    // protect init to be called only once
    std::call_once(
      _start_init_once_flag,
      [this, with_signal_handler, catchable_signals]()
      {
        _log_manager.start_backend_worker(with_signal_handler, catchable_signals);

        // Setup a handler to call stop when the main application exits.
        // always call stop on destruction to log everything. std::atexit seems to be working
        // better with dll on windows compared to using ~LogManagerSingleton().
        std::atexit([]() { LogManagerSingleton::instance().log_manager().stop_backend_worker(); });
      });
  }

private:
  LogManagerSingleton() = default;
  ~LogManagerSingleton() = default;

private:
  LogManager _log_manager;
  std::once_flag _start_init_once_flag; /** flag to start the thread only once, in case start() is called multiple times */
};
} // namespace quill::detail
