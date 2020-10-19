/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_NODISCARD
#include "quill/detail/misc/Common.h"
#include <atomic> // for atomic

namespace quill
{
namespace detail
{
/**
 * A Spinlock class
 */
class Spinlock
{
public:
  /**
   * Constructs a new spin lock.
   */
  Spinlock() = default;

  /**
   * Acquires the lock, spinning until successful.
   */
  void lock() noexcept;

  /**
   * Tries to acquire the lock, spinning until successful.
   * @return true if lock was acquired, false otherwise
   */
  QUILL_NODISCARD bool try_lock() noexcept;

  /**
   * Releases the lock.
   *
   * @note Behaviour is undefined if the lock is released from a different
   * thread to the one that acquired it.
   */
  void unlock() noexcept;

private:
  enum State : uint8_t
  {
    Free = 0,
    Locked = 1
  };

  alignas(detail::CACHELINE_SIZE) std::atomic<State> _spinlock_flag{State::Free};
};
} // namespace detail
} // namespace quill
