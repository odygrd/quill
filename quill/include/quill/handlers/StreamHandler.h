/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"                    // for memory_buffer
#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_HOT, QUILL...
#include "quill/detail/misc/Common.h"     // for filename_t
#include "quill/handlers/Handler.h"       // for Handler
#include <chrono>                         // for nanoseconds
#include <cstdio>                         // for FILE

namespace quill
{
class StreamHandler : public Handler
{
public:
  enum class StreamHandlerType
  {
    Stdout,
    Stderr,
    File
  };

  /**
   * Constructor
   * Uses the default pattern formatter
   * @param stream only stdout or stderr
   * @throws on invalid param
   */
  explicit StreamHandler(filename_t stream, FILE* file = nullptr);

  ~StreamHandler() override = default;

  /**
   * Write a formatted log record to the stream
   * @param formatted_log_record input log record to write
   * @param log_record_timestamp log record timestamp
   */
  QUILL_ATTRIBUTE_HOT void write(fmt::memory_buffer const& formatted_log_record,
                                 std::chrono::nanoseconds log_record_timestamp,
                                 LogLevel log_message_severity) override;

  /**
   * Flushes the stream
   */
  QUILL_ATTRIBUTE_HOT void flush() noexcept override;

  /**
   * @return return the name of the file
   */
  QUILL_NODISCARD virtual filename_t const& filename() const noexcept;

  /**
   * @return stdout, stderr or file based on FILE*
   */
  QUILL_NODISCARD StreamHandlerType stream_handler_type() const noexcept;

protected:
  filename_t _filename;
  FILE* _file{nullptr};
};
} // namespace quill