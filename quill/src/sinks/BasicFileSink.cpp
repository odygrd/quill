#include "quill/sinks/BasicFileSink.h"

#include <cstdio>

#include "quill/detail/CommonUtilities.h"

namespace quill
{

/***/
BasicFileSink* BasicFileSink::clone() const { return new BasicFileSink{*this}; }

/***/
void BasicFileSink::log(fmt::memory_buffer const& formatted_line)
{
  detail::fwrite_fully(formatted_line.data(), sizeof(char), formatted_line.size(), _fd);
}

/***/
void BasicFileSink::flush() { fflush(_fd); }

/***/
void BasicFileSink::_fopen(char const* filename, char const* mode)
{
#ifdef _WIN32
  _fd = ::_fsopen((filename.c_str()), mode.c_str(), _SH_DENYNO);
#else // unix
  _fd = ::fopen(filename, mode);
#endif

  if (!_fd)
  {
    throw std::system_error((errno), std::generic_category());
  }
}

void BasicFileSink::_fclose()
{
  if (_fd != nullptr)
  {
    fclose(_fd);
    _fd = nullptr;
  }
}
} // namespace quill