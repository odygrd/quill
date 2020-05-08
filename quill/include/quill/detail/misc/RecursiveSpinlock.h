/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Spinlock.h" // for Spinlock
#include <cstdint>                      // for uint16_t

namespace quill
{
namespace detail
{
/**
 * A recursive spinlock class
 */
class RecursiveSpinlock
{
public:
  /**
   * Constructs a new spin lock.
   */
  RecursiveSpinlock() = default;

  /**
   * Acquires the lock, spinning until successful.
   */
  void lock() noexcept;

  /**
   * Releases the lock.
   *
   * @note Behaviour is undefined if the lock is released from a different
   * thread to the one that acquired it.
   */
  void unlock() noexcept;

private:
  Spinlock _spinlock;
  static thread_local uint16_t _counter;
};
} // namespace detail
} // namespace quill
