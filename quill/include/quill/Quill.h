#pragma once

#include "quill/LogMacros.h"
#include "quill/LogUtilities.h"
#include "quill/detail/LogManagerSingleton.h"

namespace quill
{

/**
 * Starts the backend logging worker thread
 */
void start_logging_worker();

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
 * Creates a new Logger using the default logger's sink and logging pattern
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
 * A custom logging pattern the pattern can be specified during the sink construction before
 * the sink is passed to this function
 *
 * @note: If the user does not want to store the logger pointer, the same logger can be obtained later by calling get_logger(logger_name);
 *
 * @param logger_name
 * @param sink
 * @return A pointer to a thread-safe Logger object
 */
[[nodiscard]] Logger* create_logger(std::string logger_name, std::unique_ptr<SinkBase> sink);
} // namespace quill