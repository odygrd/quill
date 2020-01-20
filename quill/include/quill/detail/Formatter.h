#pragma once

#include <cstdint>
#include <string>

#include "fmt/format.h"
#include "quill/detail/LogLineInfo.h"

namespace quill::detail
{

class Formatter
{
public:
  /**
   * Constructor
   * @param name
   */
  Formatter() = default;

  /**
   * Deleted
   */
  Formatter(Formatter const&) = delete;
  Formatter& operator=(Formatter const&) = delete;

  /**
   * Destructor
   */
  ~Formatter() = default;

  template <typename... Args>
  fmt::memory_buffer format(uint64_t timestamp,
                            uint32_t thread_id,
                            std::string const& logger_name,
                            LogLineInfo const& logline_info,
                            Args const&... args) const
  {
    fmt::memory_buffer formatted_buffer;

    // TODO: default logger format - make this configurable
    static constexpr char const* logger_format = "{} [{}] {}:{} {} {} - ";
    // Format all logger information first
    fmt::format_to(formatted_buffer, logger_format, timestamp, thread_id, logline_info.file_name(),
                   logline_info.line(), logline_info.log_level_str(), logger_name);

    // Format the user requested string
    fmt::format_to(formatted_buffer, logline_info.format(), args...);

    // Append a new line
    formatted_buffer.push_back('\n');

    return formatted_buffer;
  }
};
} // namespace quill::detail