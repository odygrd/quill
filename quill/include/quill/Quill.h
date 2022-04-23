/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/detail/LogMacros.h"             // for filename_t
#include "quill/detail/LogManager.h"            // for LogManager
#include "quill/detail/LogManagerSingleton.h"   // for LogManagerSingleton
#include "quill/detail/backend/BackendWorker.h" // for backend_worker_error_h...
#include "quill/detail/misc/Attributes.h"       // for QUILL_ATTRIBUTE_COLD
#include "quill/detail/misc/Common.h"           // for Timezone
#include "quill/handlers/FileHandler.h"         // for FilenameAppend, Filena...
#include <chrono>                               // for hours, minutes, nanose...
#include <cstddef>                              // for size_t
#include <cstdint>                              // for uint16_t
#include <initializer_list>                     // for initializer_list
#include <string>                               // for string
#include <unordered_map>                        // for unordered_map

namespace quill
{

/** forward declarations **/
class Handler;
class Logger;

/**
 * Pre-allocates the thread-local data structures needed for the current thread.
 * Walks and pre-fetches all allocated memory
 * Although optional, it is recommended to invoke this function during the thread initialisation phase before the first log message.
 */
QUILL_ATTRIBUTE_COLD void preallocate();

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
  detail::LogManagerSingleton::instance().log_manager().start_backend_worker(with_signal_handler, catchable_signals);
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
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* stdout_handler(
  std::string const& stdout_handler_name = std::string{"stdout"},
  ConsoleColours const& console_colours = ConsoleColours{});

/**
 * @param stderr_handler_name a custom name for stdout_handler. This is only useful if you want to
 * have multiple formats in the stderr. See example_stdout_multiple_formatters.cpp example
 * @return a handler to the standard error stream
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* stderr_handler(std::string const& stderr_handler_name = std::string{"stderr"});

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
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* create_handler(filename_t const& handler_name, Args&&... args)
{
  return detail::LogManagerSingleton::instance().log_manager().handler_collection().create_handler<THandler>(
    handler_name, std::forward<Args>(args)...);
}

/**
 * Creates or returns an existing handler to a file.
 * If the file is already opened the existing handler for this file is returned instead
 * @param filename the name of the file
 * @param append_to_filename additional info to append to the name of the file.
 * FilenameAppend::None, FilenameAppend::Date, FilenameAppend::DateTime
 * @param mode Used only when the file is opened for the first time. Otherwise the value is ignored
 * If no value is specified during the file creation "a" is used as default.
 * @return A handler to a file
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* file_handler(filename_t const& filename,
                                                           std::string const& mode = std::string{"a"},
                                                           FilenameAppend append_to_filename = FilenameAppend::None);

/**
 * Creates a new instance of the TimeRotatingFileHandler class or looks up an existing instance.
 *
 * The specified file is opened and used as the stream for logging. Rotating happens based on
 * the product of when and interval. You can use the when to specify the type of interval.
 * When 'daily' is passed, the value passed for interval isn’t used.
 *
 * The system will save old log files by appending extensions to the filename. The extensions are
 * date-and-time based in the format of '%Y-%m-%d_%H-%M-%S'.
 *
 * If the timezone argument is gmt time, times in UTC will be used; otherwise local time is used
 *
 * At most backup_count files will be kept, and if more would be created when rollover occurs, the oldest one is deleted.
 *
 * at_time specifies the time of day when rollover occurs and only used when 'daily' is passed. It must be in the format HH:MM
 *
 * @param base_filename the filename
 * @param mode mode to open the file 'a' or 'w'
 * @param when 'M' for minutes, 'H' for hours or 'daily'
 * @param interval The interval used for rotation.
 * @param backup_count maximum backup files to keep
 * @param timezone if true times in UTC will be used; otherwise local time is used
 * @param at_time specifies the time of day when rollover occurs if 'daily' is passed
 * @return a pointer to a time rotating file handler
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* time_rotating_file_handler(
  filename_t const& base_filename, std::string const& mode = std::string{"a"},
  std::string const& when = std::string{"H"}, uint32_t interval = 1, uint32_t backup_count = 0,
  Timezone timezone = Timezone::LocalTime, std::string const& at_time = std::string{"00:00"});

/**
 * Creates a new instance of the RotatingFileHandler class or looks up an existing instance.
 *
 * If a rotating file handler with base_filename exists then the existing instance is returned.
 *
 * If a rotating file handler does not exist then the specified file is opened and used as the stream for logging.
 * By default, the file grows indefinitely. You can use the max_bytes and backup_count values to allow
 * the file to rollover at a predetermined size.
 * When the size is about to be exceeded, the file is closed and a new file is silently opened for output.
 *
 * Rollover occurs whenever the current log file is nearly max_bytes in length;
 * but if either of max_bytes or backup_count is zero, rollover never occurs,
 * so you generally want to set backup_count to at least 1, and have a non-zero maxBytes.
 *
 * When backup_count is non-zero, the system will save old log files by appending the
 * extensions ‘.1’, ‘.2’ etc., to the filename.
 *
 * For example, with a backup_count of 5 and a base file name of app.log,
 * you would get app.log, app.1.log, app.2.log, up to app.5.log
 * The file being written to is always app.log.
 * When this file is filled, it is closed and renamed to app.1.log, and if files
 * app.1.log, app.2.log, etc. exist, then they are renamed to app.2.log, app.3.log etc. respectively.
 *
 * @param base_filename the base file name
 * @param mode file mode to open file
 * @param max_bytes The max_bytes of the file, when the size is exceeded the file witll rollover
 * @param backup_count The maximum number of times we want to rollover
 * @return a pointer to a rotating file handler
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* rotating_file_handler(
  filename_t const& base_filename, std::string const& mode = std::string{"a"}, size_t max_bytes = 0,
  uint32_t backup_count = 0, bool overwrite_oldest_files = true);

#if defined(_WIN32)
/**
 * @see create_handler
 */
template <typename THandler, typename... Args>
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* create_handler(std::string const& handler_name, Args&&... args)
{
  return create_handler<THandler>(detail::s2ws(handler_name), std::forward<Args>(args)...);
}

/**
 * @see file_handler
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* file_handler(std::string const& filename,
                                                           std::string const& mode = std::string{"a"},
                                                           FilenameAppend append_to_filename = FilenameAppend::None);
/**
 * @see time_rotating_file_handler
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* time_rotating_file_handler(
  std::string const& base_filename, std::string const& mode = std::string{"a"},
  std::string const& when = std::string{"H"}, uint32_t interval = 1, uint32_t backup_count = 0,
  Timezone timezone = Timezone::LocalTime, std::string const& at_time = std::string{"00:00"});

/**
 * @see rotating_file_handler
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* rotating_file_handler(std::string const& base_filename,
                                                                    std::string const& mode = std::string{"a"},
                                                                    size_t max_bytes = 0,
                                                                    uint32_t backup_count = 0, 
                                                                    bool overwrite_oldest_files = true);
#endif

/**
 * Returns an existing logger given the logger name or the default logger if no arguments logger_name is passed
 *
 * @warning the logger MUST have been created first by a call to create_logger.
 *
 * It is safe calling create_logger("my_logger) and get_logger("my_logger"); in different threads but the user has
 * to make sure that the call to create_logger has returned in thread A before calling get_logger in thread B
 *
 * It is safe calling get_logger(...) in multiple threads as the same time
 *
 * @note: for efficiency prefer storing the returned Logger object pointer
 *
 * @throws when the requested logger does not exist
 *
 * @param logger_name The name of the logger, or no argument for the default logger
 * @return A pointer to a thread-safe Logger object
 */
QUILL_NODISCARD Logger* get_logger(char const* logger_name = nullptr);

/**
 * Returns all existing loggers and the pointers to them
 * @return a map logger_name -> logger*
 */
QUILL_NODISCARD std::unordered_map<std::string, Logger*> get_all_loggers();

/**
 * Creates a new Logger using the existing default logger's handler and formatter pattern
 *
 * @note: If the user does not want to store the logger pointer, the same logger can be obtained later by calling get_logger(logger_name);
 *
 * @param logger_name The name of the logger to add
 * @return A pointer to a thread-safe Logger object
 */
Logger* create_logger(char const* logger_name);

/**
 * Creates a new Logger using the custom given handler.
 *
 * A custom formatter pattern the pattern can be specified during the handler creation
 *
 * @note: If the user does not want to store the logger pointer, the same logger can be obtained later by calling get_logger(logger_name);
 *
 * @param logger_name The name of the logger to add
 * @param handler A pointer the a handler for this logger
 * @return A pointer to a thread-safe Logger object
 */
Logger* create_logger(char const* logger_name, Handler* handler);

/***
 * Creates a new Logger using the custom given handler.
 *
 * A custom formatter pattern the pattern can be specified during the handler creation for each
 * handler
 *
 * @param logger_name The name of the logger to add
 * @param handlers An initializer list of pointers to handlers for this logger
 * @return A pointer to a thread-safe Logger object
 */
Logger* create_logger(char const* logger_name, std::initializer_list<Handler*> handlers);

/**
 * Resets the default logger and re-creates the logger with the given handler
 *
 * Any loggers that are created after this point by using create_logger(std::string logger_name)
 * use the same handler by default
 *
 * This function can also be used to change the format pattern of the logger
 *
 * @warning Must be called before calling start()
 *
 * @param handler A pointer to a handler that will be now used as a default handler by the default logger
 */
QUILL_ATTRIBUTE_COLD void set_default_logger_handler(Handler* handler);

/**
 * Resets the default logger and re-creates the logger with the given multiple handlers
 *
 * Any loggers that are created after this point by using create_logger(std::string logger_name)
 * use the same multiple handlers by default
 *
 * @warning Must be called before calling start()
 *
 * @param handlers An initializer list of pointers to handlers that will be now used as a default handler by the default logger
 */
QUILL_ATTRIBUTE_COLD void set_default_logger_handler(std::initializer_list<Handler*> handlers);

/**
 * If called then by default we are printing colour codes when console/terminal is used.
 *
 * @warning Must be called before calling start()
 */
QUILL_ATTRIBUTE_COLD void enable_console_colours();

/**
 * Blocks the caller thread until all log messages up to the current timestamp are flushed
 *
 * The backend thread will call write on all handlers for all loggers up to the point (timestamp)
 * that this function was called.
 *
 * @note This function will not do anything if called while the backend worker is not running
 */
void flush();

#if !defined(QUILL_NO_EXCEPTIONS)
/**
 * The background thread in very rare occasion might thrown an exception which can not be caught in the
 * user threads. In that case the backend worker thread will call this callback instead.
 *
 * Set up a custom error handler to be used if the backend thread has any error.
 *
 * If no error handler is set, the default one will print to std::cerr.
 *
 * @note Not used when QUILL_NO_EXCEPTIONS is enabled.
 *
 * @note Must be called before quill::start();
 *
 * @param backend_worker_error_handler an error handler callback e.g [](std::string const& s) { std::cerr << s << std::endl; }
 *
 * @warning backend_worker_error_handler will be executed by the backend worker thread.
 *
 * @throws exception if it is called after the thread has started
 */

QUILL_ATTRIBUTE_COLD void set_backend_worker_error_handler(backend_worker_error_handler_t backend_worker_error_handler);
#endif

/** Runtime logger configuration options **/
namespace config
{
/**
 * Pins the backend thread to the given CPU
 *
 * By default Quill does not pin the backend thread to any CPU, unless a value is specified by
 * this function
 *
 * @param cpu The cpu affinity of the backend thread
 *
 * @warning: The backend thread will read this value when quill::start() is called.
 * This function must be called before calling quill::start() otherwise the backend thread will ignore the value.
 *
 * @see set_backend_thread_sleep_duration
 *
 * @cpu the cpu core to pin the backend thread
 */
QUILL_ATTRIBUTE_COLD void set_backend_thread_cpu_affinity(uint16_t cpu);

/**
 * Names the backend thread
 *
 * By default the backend thread is named "Quill_Backend"
 *
 * @warning: The backend thread will read this value when quill::start() is called.
 * This function must be called before calling quill::start() otherwise the backend thread will ignore the value.
 *
 * @param name The desired name of the backend worker thread
 */
QUILL_ATTRIBUTE_COLD void set_backend_thread_name(std::string const& name);

/**
 * The backend thread will always "busy wait" spinning around every caller thread's local spsc queue.
 *
 * The reason for this is to reduce latency on the caller thread as notifying the
 * backend thread even by a fast backed by atomics semaphore would add additional latency
 * to the caller thread.
 * The alternative to this is letting the backend thread "busy wait" and at the same time reduce the backend thread's
 * OS scheduler priority by a periodic call to sleep().
 *
 * Each time the backend thread sees that there are no remaining records left to process in the queues it will sleep.
 *
 * @note: It is recommended to pin the backend thread to a shared or a junk cpu core and use the
 * default sleep duration of 300ns.
 * If you really care about the backend thread speed you might want to pin that thread to an exclusive core
 * and change the sleep duration value to 0 so that the thread never sleeps
 *
 * @see set_backend_thread_cpu_affinity
 *
 * @warning: The backend thread will read this value when quill::start() is called.
 * This function must be called before calling quill::start() otherwise the backend thread will ignore the value.
 *
 * @param sleep_duration The sleep duration of the backend thread when idle
 */
QUILL_ATTRIBUTE_COLD void set_backend_thread_sleep_duration(std::chrono::nanoseconds sleep_duration);

/**
 * Sets the maximum transit events number. When that number is reached then half of them
 * will get flushed to the log files.
 * @param max_transit_events
 */
QUILL_ATTRIBUTE_COLD void set_backend_thread_max_transit_events(size_t max_transit_events);

} // namespace config

} // namespace quill
