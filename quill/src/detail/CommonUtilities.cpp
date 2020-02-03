#include "quill/detail/CommonUtilities.h"

#include <sys/syscall.h>
#include <system_error>
#include <unistd.h>

#include "quill/detail/CommonMacros.h"

namespace quill
{
namespace detail
{
/***/
uint32_t get_thread_id() noexcept { return static_cast<uint32_t>(::syscall(SYS_gettid)); }

/***/
void fwrite_fully(void const* ptr, size_t size, size_t count, FILE* stream)
{
  size_t const written = std::fwrite(ptr, size, count, stream);

  if (QUILL_UNLIKELY(written < count))
  {
    throw std::system_error((errno), std::generic_category());
  }
}
} // namespace detail
} // namespace quill