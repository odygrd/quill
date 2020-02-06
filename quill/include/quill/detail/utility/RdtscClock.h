#pragma once

#include <chrono>

#include "quill/TweakMe.h"
#include "quill/detail/CommonMacros.h"

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
    double ticks_per_ns() const noexcept { return ticks_per_ns_; }

  private:
    /**
     * Constructor
     */
    RdtscTicks();

    double ticks_per_ns_{0};
  };

public:
  /**
   * Constructor
   * @param resync_interval the interval to resync the tsc clock with the real system wall clock
   */
  explicit RdtscClock(std::chrono::nanoseconds resync_interval = std::chrono::seconds{QUILL_RDTSC_RESYNC_INTERVAL});

  /**
   * Convert tsc cycles to nanoseconds
   * @param tsc
   * @return
   */
  std::chrono::nanoseconds time_since_epoch(uint64_t rdtsc_value) const noexcept;

  /**
   * Sync base wall time and base tsc.
   * @see static constexpr std::chrono::minutes resync_timer_{5};
   */
  void resync() const noexcept;

private:
  mutable int64_t base_time_{0}; /**< Get the initial base time in nanoseconds from epoch */
  mutable uint64_t base_tsc_{0}; /**< Get the initial base tsc time */
  mutable int64_t resync_interval_ticks_{0};
  int64_t resync_interval_orginal_{0}; /**< stores the initial interval value as as if we fail to resync we increase the timer */
  double ticks_per_nanosecond_{0};
}; // namespace detailclassRdtscClock
} // namespace detail
} // namespace quill