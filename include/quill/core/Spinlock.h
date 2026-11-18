/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/ThreadPrimitives.h"

#ifndef QUILL_MODULE
  #include <atomic>
  #include <cstdint>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{
/***/
class Spinlock
{
public:
  Spinlock() = default;

  /**
   * Deleted
   */
  Spinlock(Spinlock const&) = delete;
  Spinlock& operator=(Spinlock const&) = delete;

  /**
   * Spins first, since the guarded sections are short, then yields between attempts so a waiter
   * does not burn a core while the holder is descheduled or performs a long operation.
   */
  QUILL_ATTRIBUTE_HOT void lock() noexcept
  {
    constexpr uint32_t max_spins_before_yield{1024};
    uint32_t spins{0};

    do
    {
      while (_flag.load(std::memory_order_relaxed) == State::Locked)
      {
        if (spins < max_spins_before_yield)
        {
          ++spins;
        }
        else
        {
          yield_thread();
        }
      }
    } while (_flag.exchange(State::Locked, std::memory_order_acquire) == State::Locked);
  }

  /***/
  QUILL_ATTRIBUTE_HOT void unlock() noexcept
  {
    _flag.store(State::Free, std::memory_order_release);
  }

private:
  enum class State : uint8_t
  {
    Free = 0,
    Locked = 1
  };

  std::atomic<State> _flag{State::Free};
};

/***/
class LockGuard
{
public:
  explicit LockGuard(Spinlock& s) : _spinlock(s) { _spinlock.lock(); }
  ~LockGuard() { _spinlock.unlock(); }

  /**
   * Deleted
   */
  LockGuard(LockGuard const&) = delete;
  LockGuard& operator=(LockGuard const&) = delete;

private:
  Spinlock& _spinlock;
};
} // namespace detail

QUILL_END_NAMESPACE