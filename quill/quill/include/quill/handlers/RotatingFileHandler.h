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
 * Rotating file handler based on file size
 */
class RotatingFileHandler final : public FileHandler
{
public:
  /**
   * constructor
   * @param filename Base file name to be used for logs
   * @param max_file_size max size per file in bytes
   */
  RotatingFileHandler(filename_t const& filename, size_t max_file_size);

  /**
   * Destructor
   */
  ~RotatingFileHandler() override = default;

  /**
   * Emit a formatted log record to the stream
   * @param formatted_log_record
   */
  QUILL_ATTRIBUTE_HOT void emit(fmt::memory_buffer const& formatted_log_record,
                                std::chrono::nanoseconds log_record_timestamp) override;

private:
  /**
   * Rotate to a new file
   */
  QUILL_ATTRIBUTE_COLD void _rotate();

private:
  filename_t _current_filename; /**< Includes the base filename and the index */
  size_t _current_size{0};
  size_t _max_file_size;
  uint32_t _index{0};
};

} // namespace quill