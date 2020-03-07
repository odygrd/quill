#include "quill/handlers/FileHandler.h"

#include "quill/detail/misc/FileUtilities.h"
#include "quill/detail/misc/Os.h"

namespace quill
{
/***/
FileHandler::FileHandler(filename_t const& filename, std::string const& mode)
  : StreamHandler(detail::file_utilities::open(filename, mode), filename)
{
}

/***/
FileHandler::FileHandler(filename_t const& filename) : StreamHandler(filename) {}

/***/
FileHandler::~FileHandler()
{
  if (_file)
  {
    fclose(_file);
  }
}
} // namespace quill