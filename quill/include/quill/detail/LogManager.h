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
#include <mutex> // for call_once, once_flag
#include <optional>

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
      logger_name, timestamp_clock_type.has_value() ? timestamp_clock_type.value() : _config.default_timestamp_clock_type,
      timestamp_clock.has_value() ? timestamp_clock.value() : _config.default_custom_timestamp_clock);
  }

  /***/
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, Handler* handler,
                                        std::optional<TimestampClockType> timestamp_clock_type,
                                        std::optional<TimestampClock*> timestamp_clock)
  {
    return _logger_collection.create_logger(
      logger_name, handler,
      timestamp_clock_type.has_value() ? timestamp_clock_type.value() : _config.default_timestamp_clock_type,
      timestamp_clock.has_value() ? timestamp_clock.value() : _config.default_custom_timestamp_clock);
  }

  /***/
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, std::initializer_list<Handler*> handlers,
                                        std::optional<TimestampClockType> timestamp_clock_type,
                                        std::optional<TimestampClock*> timestamp_clock)
  {
    return _logger_collection.create_logger(
      logger_name, handlers,
      timestamp_clock_type.has_value() ? timestamp_clock_type.value() : _config.default_timestamp_clock_type,
      timestamp_clock.has_value() ? timestamp_clock.value() : _config.default_custom_timestamp_clock);
  }

  /***/
  QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, std::vector<Handler*> const& handlers,
                                        std::optional<TimestampClockType> timestamp_clock_type,
                                        std::optional<TimestampClock*> timestamp_clock)
  {
    return _logger_collection.create_logger(
      logger_name, handlers,
      timestamp_clock_type.has_value() ? timestamp_clock_type.value() : _config.default_timestamp_clock_type,
      timestamp_clock.has_value() ? timestamp_clock.value() : _config.default_custom_timestamp_clock);
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

    // get the default logger - this is needed for the logger_details struct, in order to figure out
    // the clock type later on the backend thread
    Logger* default_logger = logger_collection().get_logger(nullptr);
    LoggerDetails* logger_details = std::addressof(default_logger->_logger_details);

    // Create an atomic variable
    std::atomic<bool> backend_thread_flushed{false};

    // we need to write an event to the queue passing this atomic variable
    struct
    {
      constexpr quill::MacroMetadata operator()() const noexcept
      {
        return quill::MacroMetadata{
          QUILL_STRINGIFY(__LINE__),         __FILE__, __FUNCTION__, "", LogLevel::Critical,
          quill::MacroMetadata::Event::Flush};
      }
    } anonymous_log_message_info;

    detail::ThreadContext* const thread_context = _thread_context_collection.local_thread_context();
    uint32_t total_size = sizeof(detail::Header) + sizeof(uintptr_t);

    // request this size from the queue
    std::byte* write_buffer = thread_context->spsc_queue().prepare_write(total_size);
    std::byte* const write_begin = write_buffer;

    write_buffer = detail::align_pointer<alignof(detail::Header), std::byte>(write_buffer);

    new (write_buffer) detail::Header(
      get_metadata_ptr<decltype(anonymous_log_message_info)>, logger_details,
      (logger_details->timestamp_clock_type() == TimestampClockType::Rdtsc) ? quill::detail::rdtsc()
        : (logger_details->timestamp_clock_type() == TimestampClockType::System)
        ? static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count())
        : default_logger->_custom_timestamp_clock->now());

    write_buffer += sizeof(detail::Header);

    // encode the pointer to atomic bool
    std::atomic<bool>* flush_ptr = std::addressof(backend_thread_flushed);
    std::memcpy(write_buffer, &flush_ptr, sizeof(uintptr_t));
    write_buffer += sizeof(uintptr_t);

    thread_context->spsc_queue().commit_write(static_cast<size_t>(write_buffer - write_begin));

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

private:
  void _configure()
  {
    // re-create the default logger with the given config
    _logger_collection.create_default_logger();
  }

private:
  Config _config;
  HandlerCollection _handler_collection;
  ThreadContextCollection _thread_context_collection{};
  LoggerCollection _logger_collection{_config, _thread_context_collection, _handler_collection};
  BackendWorker _backend_worker{_config, _thread_context_collection, _handler_collection};
  std::once_flag _start_init_once_flag; /** flag to start the thread only once, in case start() is called multiple times */
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
  /**
   * Access to singleton instance
   * @return a reference to the singleton
   */
  static LogManagerSingleton& instance() noexcept
  {
    static LogManagerSingleton instance;
    return instance;
  }

  /**
   * Access to LogManager
   * @return a reference to the log manager
   */
  detail::LogManager& log_manager() noexcept { return _log_manager; }

private:
  LogManagerSingleton() = default;
  ~LogManagerSingleton()
  {
    // always call stop on destruction to log everything
    _log_manager.stop_backend_worker();
  }

private:
  detail::LogManager _log_manager;
};
} // namespace detail
} // namespace quill