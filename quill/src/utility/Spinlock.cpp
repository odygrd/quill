#include "quill/detail/utility/Spinlock.h"

namespace quill::detail
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
} // namespace quill::detail