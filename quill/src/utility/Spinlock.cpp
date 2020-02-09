#include "quill/detail/misc/Spinlock.h"

namespace quill
{
namespace detail
{
/***/
void Spinlock::lock() noexcept
{
  while (_Spinlock_flag.test_and_set(std::memory_order_acquire))
  {
    // loop forever
  }
}

/***/
bool Spinlock::try_lock() noexcept
{
  return !_Spinlock_flag.test_and_set(std::memory_order_acquire);
}

/***/
void Spinlock::unlock() noexcept { _Spinlock_flag.clear(std::memory_order_release); }
} // namespace detail
} // namespace quill