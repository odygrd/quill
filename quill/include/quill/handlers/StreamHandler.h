/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"                    // for memory_buffer
#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_HOT, QUILL...
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
  explicit StreamHandler(fs::path stream, FILE* file = nullptr);

  ~StreamHandler() override = default;

  /**
   * Write a formatted log message to the stream
   * @param formatted_log_message input log message to write
   * @param log_message_timestamp log message timestamp
   */
  QUILL_ATTRIBUTE_HOT void write(fmt::memory_buffer const& formatted_log_message,
                                 std::chrono::nanoseconds log_message_timestamp,
                                 LogLevel log_message_severity) override;

  /**
   * Flushes the stream
   */
  QUILL_ATTRIBUTE_HOT void flush() noexcept override;

  /**
   * @return return the name of the file
   */
  QUILL_NODISCARD virtual fs::path const& filename() const noexcept;

  /**
   * @return stdout, stderr or file based on FILE*
   */
  QUILL_NODISCARD StreamHandlerType stream_handler_type() const noexcept;

protected:
  fs::path _filename;
  FILE* _file{nullptr};
};
} // namespace quill
