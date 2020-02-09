#pragma once

#include "quill/handlers/StreamHandler.h"

namespace quill
{
/**
 * Creates a new instance of the FileHandler class.
 * The specified file is opened and used as the stream for logging.
 * If mode is not specified, "a" is used.
 * By default, the file grows indefinitely.
 */
class FileHandler : public StreamHandler
{
public:
  explicit FileHandler(char const* filename, char const* mode = "a");

  ~FileHandler() override;

private:
  /**
   *
   * @param filename
   * @param mode
   * @throws
   * @return
   */
  QUILL_NODISCARD static FILE* _fopen(char const* filename, char const* mode);
};
} // namespace quill