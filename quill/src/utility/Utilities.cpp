#include "quill/detail/misc/Utilities.h"

#include "quill/detail/misc/Macros.h"
#include <system_error>

namespace quill
{
namespace detail
{
/***/
void fwrite_fully(void const* ptr, size_t size, size_t count, FILE* stream)
{
  size_t const written = std::fwrite(ptr, size, count, stream);

  if (QUILL_UNLIKELY(written < count))
  {
    throw std::system_error(errno, std::system_category());
  }
}
} // namespace detail
} // namespace quill