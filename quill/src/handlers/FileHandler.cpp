#include "quill/handlers/FileHandler.h"

namespace quill
{
/***/
FileHandler::FileHandler(char const* filename, char const* mode /*= "a" */)
  : StreamHandler(_fopen(filename, mode), filename){}

/***/
FileHandler::~FileHandler() { fclose(_file); }

/***/
FILE* FileHandler::_fopen(char const* filename, char const* mode)
{
  std::string file_name{filename};

  if (file_name == std::string{"stdout"} || file_name == std::string{"stderr"})
  {
    throw std::runtime_error("Invalid filename");
  }

#ifdef _WIN32
  FILE* file = ::_fsopen(filename, mode, _SH_DENYNO);
#else // unix
  FILE* file = ::fopen(filename, mode);
#endif

  if (!file)
  {
    throw std::system_error(errno, std::generic_category());
  }

  return file;
}

} // namespace quill