/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_NODISCARD
#include "quill/detail/misc/Common.h"     // for QUILL_RDTSC_RESYNC_INTERVAL
#include <chrono>                         // for nanoseconds, milliseconds
#include <cstdint>                        // for int64_t, uint64_t

namespace quill
{
namespace detail
{
/**
 * Converts tsc ticks to nanoseconds since epoch
 */
class RdtscClock
{
  /**
   * A static class that calculates the rdtsc ticks per second
   */
  class RdtscTicks
  {
  public:
    QUILL_NODISCARD static RdtscTicks& instance()
    {
      static RdtscTicks inst;
      return inst;
    }

    /***/
    double ticks_per_ns() const noexcept { return _ticks_per_ns; }

  private:
    /**
     * Constructor
     */
    RdtscTicks();

    double _ticks_per_ns{0};
  };

public:
  /**
   * Constructor
   * @param resync_interval the interval to resync the tsc clock with the real system wall clock
   */
  explicit RdtscClock(std::chrono::nanoseconds resync_interval = std::chrono::milliseconds{QUILL_RDTSC_RESYNC_INTERVAL});

  /**
   * Convert tsc cycles to nanoseconds
   * @param rdtsc_value the rdtsc timestamp to convert
   * @return the rdtsc timestamp converted to nanoseconds since epoch
   */
  std::chrono::nanoseconds time_since_epoch(uint64_t rdtsc_value) const noexcept;

  /**
   * Sync base wall time and base tsc.
   * @see static constexpr std::chrono::minutes resync_timer_{5};
   */
  void resync() const noexcept;

  /**
   * rdtsc ticks per nanosecond
   * @return the ticks per nanosecond
   */
  double ticks_per_nanosecond() const noexcept { return _ticks_per_nanosecond; }

private:
  mutable int64_t _base_time{0}; /**< Get the initial base time in nanoseconds from epoch */
  mutable uint64_t _base_tsc{0}; /**< Get the initial base tsc time */
  mutable int64_t _resync_interval_ticks{0};
  int64_t _resync_interval_original{0}; /**< stores the initial interval value as as if we fail to resync we increase the timer */
  double _ticks_per_nanosecond{0};
};
} // namespace detail
} // namespace quill