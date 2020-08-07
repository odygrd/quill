/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_NODISCARD
#include <string>                         // for string

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
  None
};

/**
 * Converts a LogLevel enum to string
 * @param log_level LogLevel
 * @return the corresponding string value
 */
QUILL_NODISCARD char const* to_string(LogLevel log_level);

/**
 * Converts a string to a LogLevel enum value
 * @param log_level the log level string to convert
 * @return the corresponding LogLevel enum value
 */
QUILL_NODISCARD LogLevel from_string(std::string log_level);

} // namespace quill