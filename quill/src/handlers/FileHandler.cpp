#include "quill/handlers/FileHandler.h"

#include "quill/detail/misc/Os.h"

namespace quill
{
/***/
FileHandler::FileHandler(filename_t filename, filename_t const& mode)
  : StreamHandler(detail::fopen(filename, mode), std::move(filename))
{
}

/***/
FileHandler::~FileHandler() { fclose(_file); }

} // namespace quill