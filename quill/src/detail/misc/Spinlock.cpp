#include "quill/detail/misc/Spinlock.h"

namespace quill
{
namespace detail
{
/***/
void Spinlock::lock() noexcept
{
  do
  {
    while (_spinlock_flag.load(std::memory_order_relaxed) == State::Locked)
    {
      // keep trying
    }
  } while (_spinlock_flag.exchange(State::Locked, std::memory_order_acquire) == State::Locked);
}

/***/
bool Spinlock::try_lock() noexcept
{
  // if we acquired the lock with exchange() that returns FREE -> 0 , and we invert to return true
  return _spinlock_flag.load(std::memory_order_relaxed) == State::Free
    ? !_spinlock_flag.exchange(State::Locked, std::memory_order_acquire)
    : false;
}

/***/
void Spinlock::unlock() noexcept { _spinlock_flag.store(State::Free, std::memory_order_release); }
} // namespace detail
} // namespace quill