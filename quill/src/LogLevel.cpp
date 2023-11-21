#include "quill/LogLevel.h"
#include "quill/QuillError.h"         // for QUILL_THROW, QuillError
#include "quill/detail/misc/Common.h" // for QUILL_UNLIKELY
#include <algorithm>                  // for transform, max
#include <array>                      // for array
#include <cctype>                     // for tolower
#include <sstream>                    // for operator<<, basic_ostream, ost...
#include <type_traits>                // for __underlying_type_impl<>::type
#include <unordered_map>              // for operator==, unordered_map, _No...

namespace quill
{
/***/
std::string_view loglevel_to_string(LogLevel log_level)
{
  static constexpr std::array<std::string_view, 11> log_levels_map = {
    {"TRACE_L3", "TRACE_L2", "TRACE_L1", "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL",
     "BACKTRACE", "NONE", "DYNAMIC"}};

  using log_lvl_t = std::underlying_type_t<LogLevel>;
  auto const log_lvl = static_cast<log_lvl_t>(log_level);

  if (QUILL_UNLIKELY(log_lvl > (log_levels_map.size() - 1)))
  {
    std::ostringstream error_msg;
    error_msg << "Invalid log_level value "
              << "\"" << log_lvl << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  return log_levels_map[log_lvl];
}

/***/
std::string_view loglevel_to_string_id(LogLevel log_level)
{
  static constexpr std::array<std::string_view, 11> log_levels_map = {
    {"T3", "T2", "T1", "D", "I", "W", "E", "C", "BT", "N", "DN"}};

  using log_lvl_t = std::underlying_type_t<LogLevel>;
  auto const log_lvl = static_cast<log_lvl_t>(log_level);

  if (QUILL_UNLIKELY(log_lvl > (log_levels_map.size() - 1)))
  {
    std::ostringstream error_msg;
    error_msg << "Invalid log_level value "
              << "\"" << log_lvl << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  return log_levels_map[log_lvl];
}

/***/
LogLevel loglevel_from_string(std::string log_level)
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
    std::ostringstream error_msg;
    error_msg << "LogLevel enum value does not exist for string "
              << "\"" << log_level << "\"";
    QUILL_THROW(QuillError{error_msg.str()});
  }

  return search->second;
}

} // namespace quill