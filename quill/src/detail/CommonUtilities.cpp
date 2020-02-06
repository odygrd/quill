#include "quill/detail/CommonUtilities.h"

#include <system_error>
#include <unistd.h>

#if defined(__linux__)
  #include <sys/syscall.h>
#elif defined(__APPLE__)
  #include <pthread.h>
#endif

#include "quill/detail/CommonMacros.h"

namespace quill
{
namespace detail
{
/***/
uint32_t get_thread_id() noexcept
{
#if defined(__linux__)
  return static_cast<uint32_t>(::syscall(SYS_gettid));
#elif defined(__APPLE__)
  uint64_t tid64;
  pthread_threadid_np(nullptr, &tid64);
  return static_cast<uint32_t>(tid64);
#endif
}

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