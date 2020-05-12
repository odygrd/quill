/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"

#include "quill/detail/BackendWorker.h"       // for backend_worker_error_h...
#include "quill/detail/LogMacros.h"           // for filename_t
#include "quill/detail/LogManager.h"          // for LogManager
#include "quill/detail/LogManagerSingleton.h" // for LogManagerSingleton
#include "quill/detail/misc/Attributes.h"     // for QUILL_ATTRIBUTE_COLD
#include "quill/handlers/FileHandler.h"       // for FilenameAppend, Filena...
#include <chrono>                             // for hours, minutes, nanose...
#include <cstddef>                            // for size_t
#include <cstdint>                            // for uint16_t
#include <initializer_list>                   // for initializer_list
#include <string>                             // for string

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
 * @throws When the backend thread fails to start
 */
QUILL_ATTRIBUTE_COLD inline void start()
{
  detail::LogManagerSingleton::instance().log_manager().start_backend_worker();
}

/**
 * @param stdout_handler_name a custom name for stdout_handler. This is only useful if you want to
 * have multiple formats in the stdout. See example_stdout_multiple_formatters.cpp example
 * @return a handler to the standard output stream
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* stdout_handler(std::string const& stdout_handler_name = std::string{"stdout"});

/**
 * @param stdout_handler_name a custom name for stdout_handler. This is only useful if you want to
 * have multiple formats in the stdout. See example_stdout_multiple_formatters.cpp example
 * @return a handler to the standard error stream
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* stderr_handler(std::string const& stdout_handler_name = std::string{"stderr"});

/**
 * Creates or returns an existing handler to a file.
 * If the file is already opened the existing handler for this file is returned instead
 * @param filename the name of the file
 * @param append_to_filename additional info to append to the name of the file. FilenameAppend::None or FilenameAppend::Date
 * @param mode Used only when the file is opened for the first time. Otherwise the value is ignored
 * If no value is specified during the file creation "a" is used as default.
 * @return A handler to a file
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* file_handler(filename_t const& filename,
                                                           std::string const& mode = std::string{},
                                                           FilenameAppend append_to_filename = FilenameAppend::None);

/**
 * Creates or returns an existing handler to a daily file handler
 * The handler will rotate creating a new log file after 24 hours based on the given rotation_hour and rotation_minute
 * @param base_filename the base filename, current date is automatically appended to this
 * @param rotation_hour The hour to perform the rotation
 * @param rotation_minute The minute to perform the rotation
 * @return A handler to a daily file
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* daily_file_handler(filename_t const& base_filename,
                                                                 std::chrono::hours rotation_hour,
                                                                 std::chrono::minutes rotation_minute);

/**
 * Creates or returns an existing handler to a rotating file handler
 * The handler will rotate creating a new log file after the file has reached max_bytes
 * @param base_filename Base file name
 * @param max_bytes Maximum file size in bytes
 * @return A handler to a rotating log file
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* rotating_file_handler(filename_t const& base_filename,
                                                                    size_t max_bytes);

#if defined(_WIN32)
/**
 * On windows filename_t always defaults to std::wstring so we provide another overload for std::string
 * Converts a std::string to std::wstring. All filesnames on windows are opened as wide strings
 * @param filename the name of the file
 * @param mode Used only when the file is opened for the first time. Otherwise the value is ignored
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* file_handler(std::string const& filename,
                                                           std::string const& mode = std::string{},
                                                           FilenameAppend append_to_filename = FilenameAppend::None);

QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* daily_file_handler(std::string const& base_filename,
                                                                 std::chrono::hours rotation_hour,
                                                                 std::chrono::minutes rotation_minute);

QUILL_NODISCARD QUILL_ATTRIBUTE_COLD Handler* rotating_file_handler(std::string const& base_filename,
                                                                    size_t max_bytes);
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
QUILL_ATTRIBUTE_COLD void set_backend_thread_cpu_affinity(uint16_t cpu) noexcept;

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
QUILL_ATTRIBUTE_COLD void set_backend_thread_name(std::string const& name) noexcept;

/**
 * The backend thread will always "busy wait" spinning around every caller thread's local spsc queue.
 *
 * The reason for this is to reduce latency on the caller thread as notifying the
 * backend thread even by a fast backed by atomics semaphone would add additional latency
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
QUILL_ATTRIBUTE_COLD void set_backend_thread_sleep_duration(std::chrono::nanoseconds sleep_duration) noexcept;

} // namespace config

} // namespace quill