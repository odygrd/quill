/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_NODISCARD
#include <string>                         // for string
#include <string_view>                    // for string_view
#include <cstdint>

namespace quill
{
/**
 * Log level enum
 */
enum class LogLevel : uint8_t
{
  TraceL3,
  TraceL2,
  TraceL1,
  Debug,
  Info,
  Warning,
  Error,
  Critical,
  Backtrace, /**< This is only used for backtrace logging. Should not be set by the user. */
  None,
  Dynamic /**< This is only used for dynamic logging. Should not be set by the user. */
};

/**
 * Converts a LogLevel enum to string
 * @param log_level LogLevel
 * @return the corresponding string value
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view loglevel_to_string(LogLevel log_level);

/**
 * Converts a LogLevel enum to string id
 * @param log_level LogLevel
 * @return the corresponding string id
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT std::string_view loglevel_to_string_id(LogLevel log_level);

/**
 * Converts a string to a LogLevel enum value
 * @param log_level the log level string to convert
 * "tracel3", "tracel2", "tracel1", "debug", "info", "warning", "error", "backtrace", "none"
 * @return the corresponding LogLevel enum value
 */
QUILL_NODISCARD LogLevel loglevel_from_string(std::string log_level);

} // namespace quill
