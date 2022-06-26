/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"                    // for memory_buffer
#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_COLD, QUIL...
#include "quill/handlers/FileHandler.h"   // for FileHandler
#include <chrono>                         // for hours, minutes, nanoseconds
#include <queue>

namespace quill
{

/**
 * Timed Rotating file handler
 */
class TimeRotatingFileHandler final : public FileHandler
{
public:
  /**
   * Constructor
   * @param base_filename base filename
   * @param mode mode to open_file file
   * @param when 'M', 'H' or 'daily'
   * @param interval Used when 'M' is 'H' is specified
   * @param backup_count Maximum files to keep
   * @param timezone if true gmt time then UTC times are used instead
   * @param at_time used when 'daily' is specified
   */
  TimeRotatingFileHandler(fs::path const& base_filename, std::string const& mode,
                          std::string when, uint32_t interval, uint32_t backup_count,
                          Timezone timezone, std::string const& at_time);

  ~TimeRotatingFileHandler() override = default;

  /**
   * Write a formatted log message to the stream
   * @param formatted_log_message input log message to write
   * @param log_message_timestamp log message timestamp
   */
  QUILL_ATTRIBUTE_HOT void write(fmt::memory_buffer const& formatted_log_message,
                                 std::chrono::nanoseconds log_message_timestamp,
                                 LogLevel log_message_severity) override;

private:
  static QUILL_ATTRIBUTE_COLD std::chrono::system_clock::time_point _calculate_initial_rotation_tp(
    std::chrono::system_clock::time_point time_now, std::string const& when, Timezone timezone,
    std::chrono::hours at_time_hours, std::chrono::minutes at_time_minutes) noexcept;

  static QUILL_ATTRIBUTE_COLD std::chrono::system_clock::time_point _calculate_rotation_tp(
    std::chrono::system_clock::time_point time_now, std::string const& when, uint32_t interval) noexcept;

private:
  std::queue<fs::path> _created_files; /**< We store in a queue the filenames we created, in order to remove_file them if we exceed _backup_count limit */
  std::string _when;                                /**< 'M', 'H' or 'daily' */
  std::chrono::system_clock::time_point _file_creation_time; /**< The time we create the file we are writing */
  std::chrono::system_clock::time_point _next_rotation_time; /**< The next rotation time point */
  uint32_t _interval;       /**< Interval when 'M' or 'H' is used */
  uint32_t _backup_count;   /**< Maximum files to keep after rotation */
  Timezone _using_timezone; /**< The timezone used */
};
} // namespace quill
