/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"                    // for memory_buffer
#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_COLD, QUIL...
#include "quill/detail/misc/Common.h"     // for filename_t
#include "quill/handlers/FileHandler.h"   // for FileHandler
#include <chrono>                         // for nanoseconds
#include <cstddef>                        // for size_t
#include <cstdint>                        // for uint32_t

namespace quill
{
/**
 * Rotating file handler based on file size
 * The specified base_filename is opened with mode 'a' and used as the stream for logging.
 * You can use the max_bytes values to allow the file to rollover at a predetermined size.
 * When the file size reaches max_bytes, the file is closed and a new file is silently opened for output.
 *  For example, with a base_filename of app.log you would get app.log, app.1.log, app.2.log etc..
 */
class RotatingFileHandler final : public FileHandler
{
public:
  /**
   * constructor
   * @param filename Base file name to be used for logs
   * @param max_bytes max size per file in bytes
   * @throws on invalid rotation values
   */
  RotatingFileHandler(filename_t const& filename, size_t max_bytes);

  /**
   * Destructor
   */
  ~RotatingFileHandler() override = default;

  /**
   * Write a formatted log record to the stream
   * @param formatted_log_record input log record to write
   * @param log_record_timestamp log record timestamp
   */
  QUILL_ATTRIBUTE_HOT void write(fmt::memory_buffer const& formatted_log_record,
                                std::chrono::nanoseconds log_record_timestamp) override;

private:
  /**
   * Rotate to a new file
   */
  QUILL_ATTRIBUTE_COLD void _rotate();

private:
  size_t _current_size{0};
  size_t _max_bytes;
  uint32_t _index{0};
};

} // namespace quill