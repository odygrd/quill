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
    // TODO: default logger format - make this configurable
    std::string const default_format_str = "{} [{}] {}:{} {} {} - ";

    std::string const fmt = default_format_str + std::string{logline_info.format()} + "\n";

    fmt::memory_buffer mem_buffer;
    fmt::format_to(mem_buffer, fmt.data(), timestamp, thread_id, logline_info.file_name(),
                   logline_info.line(), logline_info.log_level_str(), logger_name, args...);

    return mem_buffer;
  }
};
} // namespace quill::detail