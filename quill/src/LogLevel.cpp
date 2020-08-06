#include "quill/LogLevel.h"
#include "quill/QuillError.h"         // for QUILL_THROW, QuillError
#include "quill/detail/misc/Macros.h" // for QUILL_UNLIKELY
#include <algorithm>                  // for transform, max
#include <array>                      // for array
#include <cctype>                     // for tolower
#include <sstream>                    // for operator<<, basic_ostream, ost...
#include <type_traits>                // for __underlying_type_impl<>::type
#include <unordered_map>              // for operator==, unordered_map, _No...
#include <utility>                    // for pair

namespace quill
{
/***/
char const* to_string(LogLevel log_level)
{
  constexpr std::array<char const*, 10> log_levels_map = {{"TraceL3", "TraceL2", "TraceL1", "Debug",
                                                           "Info", "Warning", "Error", "Critical",
                                                           "Backtrace", "None"}};

  using log_lvl_t = std::underlying_type<LogLevel>::type;
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
LogLevel from_string(std::string log_level)
{
  static const std::unordered_map<std::string, LogLevel> log_levels_map = {
    {"tracel3", LogLevel::TraceL3},     {"tracel2", LogLevel::TraceL2},
    {"tracel1", LogLevel::TraceL1},     {"debug", LogLevel::Debug},
    {"info", LogLevel::Info},           {"warning", LogLevel::Warning},
    {"error", LogLevel::Error},         {"critical", LogLevel::Critical},
    {"backtrace", LogLevel::Backtrace}, {"none", LogLevel::None}};

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