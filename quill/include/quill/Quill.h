#pragma once

#include "quill/Macros.h"

#include "quill/Utility.h"
#include "quill/detail/LogManagerSingleton.h"

namespace quill
{
/**
 * Starts the backend thread to write the logs to the sinks
 * @throws When the backend thread fails to start
 */
void start();

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
 * @param logger_name
 * @return A pointer to a thread-safe Logger object
 */
[[nodiscard]] Logger* get_logger(std::string const& logger_name = std::string{});

/**
 * Creates a new Logger using the existing default logger's sink and formatter pattern
 *
 * @note: If the user does not want to store the logger pointer, the same logger can be obtained later by calling get_logger(logger_name);
 *
 * @param logger_name
 * @return A pointer to a thread-safe Logger object
 */
[[nodiscard]] Logger* create_logger(std::string logger_name);

/**
 * Creates a new Logger using the custom given sink.
 *
 * A custom formatter pattern the pattern can be specified during the sink construction before
 * the sink is passed to this function
 *
 * @note: If the user does not want to store the logger pointer, the same logger can be obtained later by calling get_logger(logger_name);
 *
 * @param logger_name
 * @param sink
 * @return A pointer to a thread-safe Logger object
 */
[[nodiscard]] Logger* create_logger(std::string logger_name, std::unique_ptr<SinkBase> sink);

/***
 * Creates a new Logger using the custom given sink.
 *
 * A custom formatter pattern the pattern can be specified during the sink construction for each
 * sink before the sinks are passed to this function
 *
 * @tparam TSinks
 * @param logger_name
 * @param sink
 * @param sinks
 * @return
 */
template <typename... TSinks>
[[nodiscard]] Logger* create_logger(std::string logger_name, std::unique_ptr<SinkBase> sink, TSinks&&... sinks)
{
  return detail::LogManagerSingleton::instance().log_manager().logger_collection().create_logger(
    std::move(logger_name), sink, std::forward<TSinks>(sinks)...);
}

/**
 * Resets the default logger and re-creates the logger with the given sink
 *
 * Any loggers that are created after this point by using create_logger(std::string logger_name)
 * use the same sink by default
 *
 * This function can also be used to change the format pattern of the logger
 *
 * @warning Must be called before calling start()
 *
 * @param sink
 */
void set_custom_default_logger(std::unique_ptr<SinkBase> sink);

/**
 * Resets the default logger and re-creates the logger with the given m
 *
 * Any loggers that are created after this point by using create_logger(std::string logger_name)
 * use the same multiple sinks by default
 *
 * @warning Must be called before calling start()
 *
 * @tparam TSinks
 * @param sink
 * @param sinks
 */
template <typename... TSinks>
void set_custom_default_logger(std::unique_ptr<SinkBase> sink, TSinks&&... sinks)
{
  detail::LogManagerSingleton::instance().log_manager().logger_collection().set_custom_default_logger(
    std::move(sink), std::forward<TSinks>(sinks)...);
}

/** Runtime logger configuration options **/
namespace config
{
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
 * @param sleep_duration The sleep duration
 *
 * @note: It is recommended to pin the backend thread to a shared or a junk cpu core and use the
 * default sleep duration of 500ns.
 * If you really care about the backend thread speed you might want to pin that thread to an exclusive core
 * and change the sleep duration value to 0 so that the thread never sleeps
 *
 * @see set_backend_thread_cpu_affinity
 *
 * @warning: The backend thread will read this value when quill::start() is called.
 * This function must be called before calling quill::start() otherwise the backend thread will ignore the value.
 */
void set_backend_thread_sleep_duration(std::chrono::nanoseconds sleep_duration) noexcept;

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
 */
void set_backend_thread_cpu_affinity(uint16_t cpu) noexcept;

/**
 * Names the backend thread
 *
 * By default the backend thread is named "Quill_Backend"
 *
 * @warning: The backend thread will read this value when quill::start() is called.
 * This function must be called before calling quill::start() otherwise the backend thread will ignore the value.
 *
 * @param name The desired name
 */
void set_backend_thread_name(std::string const& name) noexcept;
} // namespace config

} // namespace quill