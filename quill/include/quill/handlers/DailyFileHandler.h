/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/handlers/FileHandler.h"
#include <chrono>

namespace quill
{

/**
 * Daily Rotating file handler
 */
class DailyFileHandler final : public FileHandler
{
public:
  /**
   * Constructor
   * @param filename A base file name
   * @param rotation_hour Hour of rotation
   * @param rotation_minute Minute of rotation
   */
  DailyFileHandler(filename_t const& filename, std::chrono::hours rotation_hour, std::chrono::minutes rotation_minute);

  ~DailyFileHandler() override = default;

  /**
   * Write a formatted log record to the stream
   * @param formatted_log_record input log record to write
   * @param log_record_timestamp log record timestamp
   */
  QUILL_ATTRIBUTE_HOT void write(fmt::memory_buffer const& formatted_log_record,
                                std::chrono::nanoseconds log_record_timestamp) override;

private:
  QUILL_ATTRIBUTE_COLD void _update_rotation_tp() noexcept;

private:
  std::chrono::hours _rotation_hour;
  std::chrono::minutes _rotation_minute;
  std::chrono::system_clock::time_point _rotation_tp;
};
} // namespace quill