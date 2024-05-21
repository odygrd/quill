/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include <cstdint>

namespace quill
{
/**
 * Base class that provides a logger timestamp based on a user-provided clock source for timestamps.
 * Creating a derived class allows passing a user-generated timestamp to a Logger.
 *
 * This class is particularly useful for simulations or scenarios where time needs to be
 * manipulated, such as simulating time in the past.
 *
 * Note: The derived class must be thread-safe since the Logger object is also thread-safe,
 * unless the same logger is not used across multiple threads.
 */
class UserClockSource
{
public:
  UserClockSource() = default;

  virtual ~UserClockSource() = default;

  UserClockSource(UserClockSource const&) = delete;

  UserClockSource& operator=(UserClockSource const&) = delete;

  /**
   * Returns time since epoch in nanoseconds
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT virtual uint64_t now() const = 0;
};
} // namespace quill