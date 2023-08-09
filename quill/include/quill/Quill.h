/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/Config.h"
#include "quill/clock/TimestampClock.h"
#include "quill/detail/LogMacros.h"
#include "quill/detail/LogManager.h"            // for LogManager
#include "quill/detail/backend/BackendWorker.h" // for backend_worker_error_h...
#include "quill/detail/misc/Attributes.h"       // for QUILL_ATTRIBUTE_COLD
#include "quill/detail/misc/Common.h"           // for Timezone
#include "quill/handlers/FileHandler.h"         // for FilenameAppend, Filena...
#include "quill/handlers/JsonFileHandler.h"     // for JsonFileHandler
#include "quill/handlers/RotatingFileHandler.h" // for RotatingFileHandler
#include <chrono>                               // for hours, minutes, nanose...
#include <cstddef>                              // for size_t
#include <cstdint>                              // for uint16_t
#include <initializer_list>                     // for initializer_list
#include <limits>
#include <memory>
#include <optional>      // for optional
#include <string>        // for string
#include <unordered_map> // for unordered_map

namespace quill
{

/** Version Info **/
constexpr uint32_t VersionMajor{3};
constexpr uint32_t VersionMinor{3};
constexpr uint32_t VersionPatch{2};
constexpr uint32_t Version{VersionMajor * 10000 + VersionMinor * 100 + VersionPatch};

/** forward declarations **/
class Handler;
class Logger;

/**
 * Pre-allocates the thread-local data needed for the current thread.
 * Although optional, it is recommended to invoke this function during the thread initialisation
 * phase before the first log message.
 */
QUILL_ATTRIBUTE_COLD inline void preallocate()
{
  QUILL_MAYBE_UNUSED uint32_t const volatile x = detail::LogManagerSingleton::instance()
                                                   .log_manager()
                                                   .thread_context_collection()
                                                   .local_thread_context<QUILL_QUEUE_TYPE>()
                                                   ->spsc_queue<QUILL_QUEUE_TYPE>()
                                                   .capacity();
}

/**
 * Applies the given config to the logger
 * @param config configuration
 * @note Has to be called before quill::start()
 */
QUILL_ATTRIBUTE_COLD void configure(Config const& config);

/**
 * Starts the backend thread to write the logs to the handlers.
 * This function is protected with a std::call_once flag, it can only be called once.
 * Blocks the caller thread until the backend worker thread starts spinning.
 *
 * @param with_signal_handler Initialises a signal handler (or exception handler and Ctrl-C on Windows)
 *                            that will catch signals and flush the log before the application crashes
 *
 * @param catchable_signals List of the signals that the signal handler will catch (Posix ONLY).
 *
 * @note On Windows regardless the value of `with_signal_handler` init_signal_handler
 * can also be called on each new caller thread. @see init_signal_handler
 * - with_signal_handler=true will set up an exception handler and a Ctrl-C on windows to handle windows exception.
 * - init_signal_handler() on Windows will setup a signal handler that will handle posix style signals.
 * To fully handle signals on windows use with_signal_handler=true and also call init_signal_handler()
 * on each thread.
 *
 * @throws When the backend thread fails to start
 */
QUILL_ATTRIBUTE_COLD inline void start(bool with_signal_handler = false,
                                       std::initializer_list<int> catchable_signals = {
                                         SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV})
{
  detail::LogManagerSingleton::instance().start_backend_worker(with_signal_handler, catchable_signals);
}

#if defined(_WIN32)
/**
 * Setups up a signal handler for the caller thread. This must be called by each new thread
 * on windows. On linux this is called automatically on quill::start().
 * When init_signal_handler() is not called on windows, the windows exception will be caught
 * instead if start() was called with_signal_handler = true
 * @param catchable_signals List of the signals that the signal handler will catch
 */
QUILL_ATTRIBUTE_COLD inline void init_signal_handler(std::initializer_list<int> catchable_signals = {
                                                       SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV})
{
  quill::detail::init_signal_handler(catchable_signals);
}
#endif

/**
 * @param stdout_handler_name a custom name for stdout_handler. This is only useful if you want to
 * have multiple formats in the stdout. See example_stdout_multiple_formatters.cpp example
 *  @param console_colours a console colours configuration class
 * @return a handler to the standard output stream
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> stdout_handler(
  std::string const& stdout_handler_name = std::string{"stdout"},
  ConsoleColours const& console_colours = ConsoleColours{});

/**
 * @param stderr_handler_name a custom name for stdout_handler. This is only useful if you want to
 * have multiple formats in the stderr. See example_stdout_multiple_formatters.cpp example
 * @return a handler to the standard error stream
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> stderr_handler(
  std::string const& stderr_handler_name = std::string{"stderr"});

/**
 * Creates new handler and registers it internally.
 * This can be also used for creating custom handlers.
 * If a handler is already registered under the same name the existing handler is returned and
 * no new handler is created.
 * @tparam THandler type of the handler
 * @tparam Args the handler's constructor arguments types
 * @param handler_name the name of the handler
 * @param args the handler's constructor arguments, excluding the file name
 * @return A pointer to a new or existing handler
 */
template <typename THandler, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> create_handler(std::string const& handler_name,
                                                                             Args&&... args)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().create_handler<THandler>(
    handler_name, std::forward<Args>(args)...);
}

/**
 * Returns an existing handler by name
 * @param handler_name the name of the handler
 * @throws std::runtime_error if the handler does not exist
 * @return A pointer to the handler
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> get_handler(std::string const& handler_name);

/**
 * Creates or returns an existing handler to a file.
 * If the file is already opened the existing handler for this file is returned instead.
 *
 * @note It is possible to remove the file handler and close the associated file by removing all the loggers
 * associated with this handler with `quill::remove_logger()`
 *
 * @param filename the name of the file
 * @param config configuration for the file handler
 * @param file_event_notifier a FileEventNotifier to get callbacks to file events such as before_open, after_open etc
 * @return A handler to a file
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> file_handler(
  fs::path const& filename, FileHandlerConfig const& config = FileHandlerConfig{},
  FileEventNotifier file_event_notifier = FileEventNotifier{});

/**
 * Creates a new instance of the RotatingFileHandler class.
 * If the file is already opened the existing handler for this file is returned instead.
 *
 * @note It is possible to remove the file handler and close the associated file by removing all the loggers
 * associated with this handler with `quill::remove_logger()`
 *
 * @param base_filename the base file name
 * @param config configuration for the rotating file handler
 * @param file_event_notifier a FileEventNotifier to get callbacks to file events such as before_open, after_open etc
 * @return a pointer to a rotating file handler
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> rotating_file_handler(
  fs::path const& base_filename, RotatingFileHandlerConfig const& config = RotatingFileHandlerConfig{},
  FileEventNotifier file_event_notifier = FileEventNotifier{});

/**
 * Creates a new instance of the JsonFileHandler.
 * If the file is already opened the existing handler for this file is returned instead.
 *
 * When the JsonFileHandler is used named arguments need to be passed as the format string
 * to the loggers. See examples/example_json_structured_log.cpp
 *
 * @note It is possible to remove the file handler and close the associated file by removing all the loggers
 * associated with this handler with `quill::remove_logger()`
 *
 * @param filename the name of the file
 * @param config configuration for the json file handler
 * @param file_event_notifier a FileEventNotifier to get callbacks to file events such as before_open, after_open etc
 * @return a pointer to a json file handler
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> json_file_handler(
  fs::path const& filename, JsonFileHandlerConfig const& config = JsonFileHandlerConfig{},
  FileEventNotifier file_event_notifier = FileEventNotifier{});

/**
 * Creates a new instance of a NullHandler. The null handler does not do any formatting or output.
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::shared_ptr<Handler> null_handler();

/**
 * Returns an existing logger given the logger name or the root logger if no arguments logger_name is passed.
 * This function is also thread safe.
 *
 * @warning the logger MUST have been created first by a call to create_logger.
 *
 * It is safe calling create_logger("my_logger) and get_logger("my_logger") in different threads but the user has
 * to make sure that the call to create_logger has returned in thread A before calling get_logger in thread B
 *
 * @note: for efficiency prefer storing the returned Logger* when get_logger("...") is used. Multiple calls to ``get_logger(name)`` will slow your code down since it will first use a lock mutex lock and then perform a look up. The advise is to store a ``quill::Logger*`` and use that pointer directly, at least in code hot paths.
 *
 * @note: safe to call even before even calling `quill:start()` unlike using `get_root_logger()`
 *
 * @throws when the requested logger does not exist
 *
 * @param logger_name The name of the logger, or no argument for the root logger
 * @return A pointer to a thread-safe Logger object
 */
QUILL_NODISCARD Logger* get_logger(char const* logger_name = nullptr);

/**
 * Provides fast access to the root logger.
 * @note: This function can be used in the hot path and is more efficient than calling get_logger(nullptr)
 * @warning This should be used only after calling quill::start(); if you need the root logger earlier then call get_logger() instead
 * @return pointer to the root logger
 */
QUILL_NODISCARD Logger* get_root_logger() noexcept;

/**
 * Returns all existing loggers and the pointers to them
 * @return a map logger_name -> logger*
 */
QUILL_NODISCARD std::unordered_map<std::string, Logger*> get_all_loggers();

/**
 * Creates a new Logger using the existing root logger's handler and formatter pattern
 *
 * @note: If the user does not want to store the logger pointer, the same logger can be obtained later by calling get_logger(logger_name);
 *
 * @param logger_name The name of the logger to add
 * @param timestamp_clock_type rdtsc, chrono or custom clock
 * @param timestamp_clock custom user clock
 * @return A pointer to a thread-safe Logger object
 */
QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                      std::optional<TimestampClockType> timestamp_clock_type = std::nullopt,
                                      std::optional<TimestampClock*> timestamp_clock = std::nullopt);

/**
 * Creates a new Logger using the custom given handler.
 *
 * A custom formatter pattern the pattern can be specified during the handler creation
 *
 * @note: If the user does not want to store the logger pointer, the same logger can be obtained later by calling get_logger(logger_name);
 *
 * @param logger_name The name of the logger to add
 * @param handler A pointer the a handler for this logger
 * @param timestamp_clock_type rdtsc, chrono or custom clock
 * @param timestamp_clock custom user clock
 * @return A pointer to a thread-safe Logger object
 */
QUILL_NODISCARD Logger* create_logger(std::string const& logger_name, std::shared_ptr<Handler>&& handler,
                                      std::optional<TimestampClockType> timestamp_clock_type = std::nullopt,
                                      std::optional<TimestampClock*> timestamp_clock = std::nullopt);

/**
 * Creates a new Logger using the custom given handler.
 *
 * A custom formatter pattern the pattern can be specified during the handler creation for each
 * handler
 *
 * @param logger_name The name of the logger to add
 * @param handlers An initializer list of pointers to handlers for this logger
 * @param timestamp_clock_type rdtsc, chrono or custom clock
 * @param timestamp_clock custom user clock
 * @return A pointer to a thread-safe Logger object
 */
QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                      std::initializer_list<std::shared_ptr<Handler>> handlers,
                                      std::optional<TimestampClockType> timestamp_clock_type = std::nullopt,
                                      std::optional<TimestampClock*> timestamp_clock = std::nullopt);

/**
 * Creates a new Logger using the custom given handler.
 *
 * A custom formatter pattern the pattern can be specified during the handler creation for each
 * handler
 *
 * @param logger_name The name of the logger to add
 * @param handlers A vector of pointers to handlers for this logger
 * @param timestamp_clock_type rdtsc, chrono or custom clock
 * @param timestamp_clock custom user clock
 * @return A pointer to a thread-safe Logger object
 */
QUILL_NODISCARD Logger* create_logger(std::string const& logger_name,
                                      std::vector<std::shared_ptr<Handler>>&& handlers,
                                      std::optional<TimestampClockType> timestamp_clock_type = std::nullopt,
                                      std::optional<TimestampClock*> timestamp_clock = std::nullopt);

/**
 * Removes the logger. The logger is async removed by the backend logging thread after
 * all pending messages are processed.
 * When a logger is removed the associated Handler will also get removed if no other logger is using it.
 * @warning This function is thread safe but the user has to make sure they do not log
 * anything else from that logger after calling this function. There is no check on the hot
 * path that the logger is invalidated other than an assertion.
 * @param logger the pointer to the logger to remove
 */
void remove_logger(Logger* logger);

/**
 * Blocks the caller thread until all log messages up to the current timestamp are flushed
 *
 * The backend thread will call write on all handlers for all loggers up to the point (timestamp)
 * that this function was called.
 *
 * @note This function will not do anything if called while the backend worker is not running
 */
inline void flush() { detail::LogManagerSingleton::instance().log_manager().flush(); }

/**
 * Wakes up the backend logging thread on demand.
 * The backend logging thread busy waits by design.
 * A use case for this is when you do want the backend logging thread to consume too much CPU
 * you can configure it to sleep for a long amount of time and wake it up on demand to log.
 * (e.g. cfg.backend_thread_sleep_duration = = std::chrono::hours {240};)
 * This is thread safe and can be called from any thread.
 */
void wake_up_logging_thread();

} // namespace quill
