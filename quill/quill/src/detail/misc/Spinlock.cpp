#include "quill/detail/misc/Spinlock.h"

namespace quill
{
namespace detail
{
/***/
void Spinlock::lock() noexcept
{
  while (_spinlock_flag.load(std::memory_order_relaxed) == State::Locked)
  {
    // keep trying
  }

  // here we can try to acquire the lock
  State old_state = _spinlock_flag.exchange(State::Locked, std::memory_order_acquire);

  while (old_state == State::Locked)
  {
    // We failed to acquire the lock
    while (_spinlock_flag.load(std::memory_order_relaxed) == State::Locked)
    {
      // keep trying
    }
    old_state = _spinlock_flag.exchange(State::Locked, std::memory_order_acquire);
  }
}

/***/
bool Spinlock::try_lock() noexcept
{
  // we want 0 to be returned meaning the lock was previously free and invert it to return true that lock was acquired
  return _spinlock_flag.load(std::memory_order_relaxed) == State::Free
    ? !_spinlock_flag.exchange(State::Locked, std::memory_order_acquire)
    : false;
}

/***/
void Spinlock::unlock() noexcept { _spinlock_flag.store(State::Free, std::memory_order_release); }
} // namespace detail
} // namespace quill