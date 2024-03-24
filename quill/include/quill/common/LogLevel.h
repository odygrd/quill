/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/common/Attributes.h"
#include "quill/common/Common.h"
#include "quill/common/QuillError.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

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
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::string_view loglevel_to_string(LogLevel log_level)
{
  static constexpr std::array<std::string_view, 11> log_levels_map = {
    {"TRACE_L3", "TRACE_L2", "TRACE_L1", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL",
     "BACKTRACE", "NONE", "DYNAMIC"}};

  using log_lvl_t = std::underlying_type_t<LogLevel>;
  auto const log_lvl = static_cast<log_lvl_t>(log_level);

  if (QUILL_UNLIKELY(log_lvl > (log_levels_map.size() - 1)))
  {
    std::string const error_msg = "Invalid log_level value \"" + std::to_string(log_lvl) + "\"";
    QUILL_THROW(QuillError{error_msg});
  }

  return log_levels_map[log_lvl];
}

/**
 * Converts a LogLevel enum to string id
 * @param log_level LogLevel
 * @return the corresponding string id
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_HOT inline std::string_view loglevel_to_string_id(LogLevel log_level)
{
  static constexpr std::array<std::string_view, 11> log_levels_map = {
    {"T3", "T2", "T1", "D", "I", "W", "E", "C", "BT", "N", "DN"}};

  using log_lvl_t = std::underlying_type_t<LogLevel>;
  auto const log_lvl = static_cast<log_lvl_t>(log_level);

  if (QUILL_UNLIKELY(log_lvl > (log_levels_map.size() - 1)))
  {
    std::string const error_msg = "Invalid log_level value \"" + std::to_string(log_lvl) + "\"";
    QUILL_THROW(QuillError{error_msg});
  }

  return log_levels_map[log_lvl];
}

/**
 * Converts a string to a LogLevel enum value
 * @param log_level the log level string to convert
 * "tracel3", "tracel2", "tracel1", "debug", "info", "warning", "error", "backtrace", "none"
 * @return the corresponding LogLevel enum value
 */
QUILL_NODISCARD LogLevel inline loglevel_from_string(std::string log_level)
{
  static std::unordered_map<std::string, LogLevel> const log_levels_map = {
    {"tracel3", LogLevel::TraceL3},   {"trace_l3", LogLevel::TraceL3},
    {"tracel2", LogLevel::TraceL2},   {"trace_l2", LogLevel::TraceL2},
    {"tracel1", LogLevel::TraceL1},   {"trace_l1", LogLevel::TraceL1},
    {"debug", LogLevel::Debug},       {"info", LogLevel::Info},
    {"warning", LogLevel::Warning},   {"error", LogLevel::Error},
    {"critical", LogLevel::Critical}, {"backtrace", LogLevel::Backtrace},
    {"none", LogLevel::None},         {"dynamic", LogLevel::Dynamic}};

  // parse log level
  std::transform(log_level.begin(), log_level.end(), log_level.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  auto const search = log_levels_map.find(log_level);

  if (QUILL_UNLIKELY(search == log_levels_map.cend()))
  {
    std::string const error_msg = "LogLevel enum value does not exist for \"" + log_level + "\"";
    QUILL_THROW(QuillError{error_msg});
  }

  return search->second;
}
} // namespace quill
