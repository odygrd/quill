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
  explicit FileHandler(filename_t const& filename, std::string const& mode);

  ~FileHandler() override;
};
} // namespace quill