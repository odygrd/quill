/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#ifndef QUILL_MODULE
  #include <cstdint>
#endif

QUILL_BEGIN_NAMESPACE

/**
 * @brief Base class that provides a timestamp for log statements based on a user-provided clock source.
 *
 * This base class can be derived from to pass a user-generated timestamp to a Logger.
 *
 * It is particularly useful for simulations or scenarios where time manipulation is necessary,
 * such as simulating time in the past and displaying past timestamps in logs. It can also supply
 * raw TSC ticks or epoch nanoseconds by reporting the corresponding TimestampType.
 *
 * @note now() runs on logging threads; timestamp_type() runs on the backend and may overlap with
 * now() or application clock updates. Keep the timestamp representation fixed while the logger
 * exists, and synchronize shared mutable clock state as needed.
 */
QUILL_BEGIN_EXPORT

class UserClockSource
{
public:
  UserClockSource() = default;
  virtual ~UserClockSource() = default;

  UserClockSource(UserClockSource const&) = delete;
  UserClockSource& operator=(UserClockSource const&) = delete;

  /**
   * Returns a timestamp in the representation reported by timestamp_type().
   *
   * Quill calls now() after encoding a record and immediately before committing it to the
   * calling thread's queue. It must return normally and must not log through Quill, even
   * through a different logger. Loggers on one thread share that queue.
   *
   * @note Values returned by successive calls to now() should be monotonically non-decreasing.
   *       For custom epoch timestamps, the backend bypasses strict ordering. A clock that steps
   *       backwards can therefore produce log lines that appear out of timestamp order.
   *
   * @return The current timestamp as raw TSC ticks or epoch nanoseconds
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT virtual uint64_t now() const noexcept = 0;

  /**
   * Defaults to epoch nanoseconds, the common case, preserving existing clock implementations.
   * Override for raw TSC ticks; keep the format fixed for the logger and its queued records.
   */
  QUILL_NODISCARD virtual TimestampType timestamp_type() const noexcept
  {
    return TimestampType::EpochNanoseconds;
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
