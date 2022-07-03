/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Os.h"

namespace quill
{
/**
 * Base class that provides a logger timestamp based on a custom clock
 * Creating a derived class allows to pass a custom timestamp to a Logger
 * Note: The derived class must be thread-safe since the Logger object is also thread-safe.
 */
class TimestampClock
{
public:
  TimestampClock() = default;
  virtual ~TimestampClock() = default;

  TimestampClock(TimestampClock const&) = delete;
  TimestampClock& operator=(TimestampClock const&) = delete;

  /**
   * Returns time since epoch in nanoseconds
   */
  QUILL_ATTRIBUTE_HOT virtual uint64_t now() const = 0;
};
} // namespace quill