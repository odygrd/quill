#pragma once

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
   * @throws on invalid param
   */
  explicit StreamHandler(std::string stream);

  ~StreamHandler() override = default;

  /**
   * Emit a formatted log record to the stream
   * @param formatted_log_record
   */
  void emit(PatternFormatter::log_record_memory_buffer const& formatted_log_record) override;

  /**
   * Flushes the stream
   */
  void flush() override;

  QUILL_NODISCARD virtual std::string const& filename() const noexcept;

protected:
  StreamHandler(FILE* file_pointer, char const* filename);

protected:
  std::string _filename;
  FILE* _file{nullptr};
};
} // namespace quill