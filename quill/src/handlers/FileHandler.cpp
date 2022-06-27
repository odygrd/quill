#include "quill/handlers/FileHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for append_date_to_filename
#include <cstdio>                            // for fclose

namespace quill
{
/***/
FileHandler::FileHandler(fs::path const& filename, std::string const& mode, FilenameAppend append_to_filename)
  : StreamHandler(filename)
{
  if (append_to_filename == FilenameAppend::None)
  {
    _current_filename = filename;
  }
  else if (append_to_filename == FilenameAppend::Date)
  {
    _current_filename = detail::append_date_to_filename(_filename);
  }
  else if (append_to_filename == FilenameAppend::DateTime)
  {
    _current_filename = detail::append_date_to_filename(_filename, std::chrono::system_clock::now(), true);
  }

  // _file is the base file*
  _file = detail::open_file(_current_filename, mode);
}

/***/
FileHandler::FileHandler(fs::path const& filename) : StreamHandler(filename) {}

/***/
FileHandler::~FileHandler()
{
  if (_file)
  {
    fclose(_file);
  }
}
} // namespace quill