#include "quill/detail/utility/RdtscClock.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <x86intrin.h>

#include "quill/detail/CommonMacros.h"

namespace quill
{
namespace detail
{
/***/
RdtscClock::RdtscTicks::RdtscTicks()
{
  // Convert rdtsc to wall time.
  // 1. Get real time and rdtsc current count
  // 2. Calculate how many rdtsc ticks can occur in one
  // calculate _ticks_per_ns as the median over a number of observations.
  constexpr std::chrono::milliseconds spin_duration = std::chrono::milliseconds{10};

  constexpr int trials = 13;
  std::array<double, trials> rates = {0};

  for (int i = 0; i < trials; ++i)
  {
    std::chrono::nanoseconds beg_ts, end_ts;
    uint64_t beg_tsc, end_tsc;

    beg_ts = std::chrono::nanoseconds{std::chrono::steady_clock::now().time_since_epoch().count()};
    beg_tsc = __rdtsc();

    std::chrono::nanoseconds elapsed_ns;
    do
    {
      end_ts = std::chrono::nanoseconds{std::chrono::steady_clock::now().time_since_epoch().count()};
      end_tsc = __rdtsc();

      elapsed_ns = end_ts - beg_ts;       // calculates ns between two timespecs
    } while (elapsed_ns < spin_duration); // busy spin for 10ms

    rates[i] = static_cast<double>(end_tsc - beg_tsc) / elapsed_ns.count();
  }

  std::nth_element(rates.begin(), rates.begin() + trials / 2, rates.end());

  ticks_per_ns_ = rates[trials / 2];
}

double ticks_per_ns_{0};

/***/
RdtscClock::RdtscClock(std::chrono::nanoseconds resync_interval /* = std::chrono::seconds{100} */)
  : resync_interval_ticks_(
      static_cast<std::uint64_t>(resync_interval.count() * RdtscTicks::instance().ticks_per_ns())),
    resync_interval_orginal_(resync_interval_ticks_),
    ticks_per_nanosecond_(RdtscTicks::instance().ticks_per_ns())

{
  resync();
}

/**
 * Convert tsc cycles to nanoseconds
 * @param tsc
 * @return
 */
std::chrono::nanoseconds RdtscClock::time_since_epoch(uint64_t rdtsc_value) const noexcept
{
  // get rtsc current value and compare the diff then add it to base wall time
  int64_t const diff = rdtsc_value - base_tsc_;

  auto const duration_since_epoch =
    std::chrono::nanoseconds{base_time_ + static_cast<int64_t>(diff / ticks_per_nanosecond_)};

  // we need to sync after we calculated otherwise base_tsc value will be ahead of passed tsc value
  if (diff > resync_interval_ticks_)
  {
    resync();
  }

  return duration_since_epoch;
}

/**
 * Sync base wall time and base tsc.
 * @see static constexpr std::chrono::minutes resync_timer_{5};
 */
void RdtscClock::resync() const noexcept
{
  // Sometimes we might get an interrupt and might never resync so we will try again up to max_attempts
  constexpr uint8_t max_attempts{4};

  for (uint8_t attempt = 0; attempt < max_attempts; ++attempt)
  {
    uint64_t const beg = __rdtsc();
    // we force convert to nanoseconds because the precision of system_clock::time-point is not portable across platforms.
    uint64_t const wall_time = static_cast<uint64_t>(std::chrono::nanoseconds{
            std::chrono::system_clock::now().time_since_epoch()}.count());
    uint64_t const end = __rdtsc();

    if (QUILL_LIKELY(end - beg <= 2000))
    {
      base_time_ = wall_time;
      base_tsc_ = end;
      resync_interval_ticks_ = resync_interval_orginal_;
      return;
    }
  }

  // we failed to return earlier and we never resynced but we don't really want to keep retrying on each call
  // to time_since_epoch() so we do non accurate resync we will increace the resynce duration to resync later
  resync_interval_ticks_ = resync_interval_ticks_ * 2;
}
} // namespace detail
} // namespace quill