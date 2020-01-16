#include "quill/detail/Utilities.h"

#include <sys/syscall.h>
#include <unistd.h>

namespace quill::detail
{

/***/
uint32_t get_thread_id() noexcept { return static_cast<uint32_t>(::syscall(SYS_gettid)); }

} // namespace quill::detail