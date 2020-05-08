#include "quill/handlers/FileHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for append_date_to_filename
#include <cstdio>                            // for fclose

namespace quill
{
/***/
FileHandler::FileHandler(filename_t const& filename, std::string const& mode, FilenameAppend append_to_filename)
  : StreamHandler(filename)
{
  if (append_to_filename == FilenameAppend::None)
  {
    _current_filename = filename;
  }
  else if (append_to_filename == FilenameAppend::Date)
  {
    _current_filename = detail::file_utilities::append_date_to_filename(_filename);
  }

  // _file is the base file*
  _file = detail::file_utilities::open(_current_filename, mode);
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