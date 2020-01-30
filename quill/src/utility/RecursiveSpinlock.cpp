#include "quill/detail/utility/RecursiveSpinlock.h"

#include <cstdint>

namespace quill::detail
{

thread_local uint16_t RecursiveSpinlock::_counter = 0;

void RecursiveSpinlock::lock() noexcept
{
  if (_counter == 0)
  {
    // we haven't locked it yet
    _spinlock.lock();
  }
  ++_counter;
}

void RecursiveSpinlock::unlock() noexcept
{
  --_counter;
  if (_counter == 0)
  {
    _spinlock.unlock();
  }
}
} // namespace quill::detail