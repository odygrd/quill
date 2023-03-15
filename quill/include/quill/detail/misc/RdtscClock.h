/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_NODISCARD
#include "quill/detail/misc/Common.h"
#include <array>
#include <atomic>
#include <chrono>  // for nanoseconds, milliseconds
#include <cstdint> // for int64_t, uint64_t

namespace quill::detail
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
    QUILL_NODISCARD double ns_per_tick() const noexcept { return _ns_per_tick; }

  private:
    /**
     * Constructor
     */
    RdtscTicks();

    double _ns_per_tick{0};
  };

public:
  /**
   * Constructor
   * @param resync_interval the interval to resync the tsc clock with the real system wall clock
   */
  explicit RdtscClock(std::chrono::nanoseconds resync_interval);

  /**
   * Convert tsc cycles to nanoseconds
   * @param rdtsc_value the rdtsc timestamp to convert
   * @return the rdtsc timestamp converted to nanoseconds since epoch
   * @note should only get called by the backend logging thread
   */
  uint64_t time_since_epoch(uint64_t rdtsc_value) const noexcept;

  /**
   * Convert tsc cycles to nanoseconds
   * @param rdtsc_value the rdtsc timestamp to convert
   * @return the rdtsc timestamp converted to nanoseconds since epoch
   * @note thread-safe, can be called by anyone
   */
  uint64_t time_since_epoch_safe(uint64_t rdtsc_value) const noexcept;

  /**
   * Sync base wall time and base tsc.
   * @see static constexpr std::chrono::minutes resync_timer_{5};
   */
  bool resync(uint32_t lag) const noexcept;

  /**
   * rdtsc ticks per nanosecond
   * @return the ticks per nanosecond
   */
  double nanoseconds_per_tick() const noexcept { return _ns_per_tick; }

private:
  struct BaseTimeTsc
  {
    BaseTimeTsc() = default;
    int64_t base_time{0}; /**< Get the initial base time in nanoseconds from epoch */
    uint64_t base_tsc{0}; /**< Get the initial base tsc time */
  };

private:
  mutable int64_t _resync_interval_ticks{0};
  int64_t _resync_interval_original{0}; /**< stores the initial interval value as as if we fail to resync we increase the timer */
  double _ns_per_tick{0};

  alignas(CACHE_LINE_ALIGNED) mutable std::atomic<uint32_t> _version{0};
  mutable std::array<BaseTimeTsc, 2> _base{};
};
} // namespace quill::detail