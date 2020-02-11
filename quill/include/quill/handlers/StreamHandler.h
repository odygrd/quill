#pragma once

#include "quill/detail/misc/Common.h"
#include "quill/handlers/Handler.h"
#include <cstdio>
#include <string>

namespace quill
{
class StreamHandler : public Handler
{
public:
  /**
   * Constructor
   * Uses the default pattern formatter
   * @param stream only stdout or stderr
   * @throws on invalid param
   */
  explicit StreamHandler(filename_t stream);

  ~StreamHandler() override = default;

  /**
   * Emit a formatted log record to the stream
   * @param formatted_log_record
   */
  void emit(fmt::memory_buffer const& formatted_log_record) override;

  /**
   * Flushes the stream
   */
  void flush() override;

  QUILL_NODISCARD virtual filename_t const& filename() const noexcept;

protected:
  /**
   * Protected contructor used by the file handler
   * @param file_pointer
   * @param filename
   */
  StreamHandler(FILE* file_pointer, filename_t filename);

protected:
  filename_t _filename;
  FILE* _file{nullptr};
};
} // namespace quill