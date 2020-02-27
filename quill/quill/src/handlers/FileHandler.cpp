#include "quill/handlers/FileHandler.h"

#include "quill/detail/misc/Os.h"

namespace quill
{
/***/
FileHandler::FileHandler(filename_t const& filename, std::string const& mode)
  : StreamHandler(detail::fopen(filename, mode), filename)
{
}

/***/
FileHandler::~FileHandler() { fclose(_file); }

} // namespace quill