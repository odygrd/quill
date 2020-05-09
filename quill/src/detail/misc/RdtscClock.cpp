#include "quill/detail/misc/RdtscClock.h"
#include "quill/detail/misc/Macros.h" // for QUILL_LIKELY
#include "quill/detail/misc/Rdtsc.h"  // for rdtsc
#include <algorithm>                  // for nth_element
#include <array>                      // for array<>::iterator, array
#include <chrono>                     // for nanoseconds, duration, operator-
#include <cstddef>                    // for size_t

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
  std::array<double, trials> rates = {{0}};

  for (size_t i = 0; i < trials; ++i)
  {
    auto const beg_ts =
      std::chrono::nanoseconds{std::chrono::steady_clock::now().time_since_epoch().count()};
    uint64_t const beg_tsc = rdtsc();

    std::chrono::nanoseconds elapsed_ns;
    uint64_t end_tsc;
    do
    {
      auto const end_ts =
        std::chrono::nanoseconds{std::chrono::steady_clock::now().time_since_epoch().count()};
      end_tsc = rdtsc();

      elapsed_ns = end_ts - beg_ts;       // calculates ns between two timespecs
    } while (elapsed_ns < spin_duration); // busy spin for 10ms

    rates[i] = static_cast<double>(end_tsc - beg_tsc) / static_cast<double>(elapsed_ns.count());
  }

  std::nth_element(rates.begin(), rates.begin() + trials / 2, rates.end());

  _ticks_per_ns = rates[trials / 2];
}

/***/
RdtscClock::RdtscClock(std::chrono::nanoseconds resync_interval /* = std::chrono::seconds{100} */)
  : _resync_interval_ticks(static_cast<std::int64_t>(static_cast<double>(resync_interval.count()) *
                                                     RdtscTicks::instance().ticks_per_ns())),
    _resync_interval_original(_resync_interval_ticks),
    _ticks_per_nanosecond(RdtscTicks::instance().ticks_per_ns())

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
  auto const diff = static_cast<int64_t>(rdtsc_value - _base_tsc);

  auto const duration_since_epoch = std::chrono::nanoseconds{
    _base_time + static_cast<int64_t>(static_cast<double>(diff) / _ticks_per_nanosecond)};

  // we need to sync after we calculated otherwise base_tsc value will be ahead of passed tsc value
  if (diff > _resync_interval_ticks)
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
    uint64_t const beg = rdtsc();
    // we force convert to nanoseconds because the precision of system_clock::time-point is not portable across platforms.
    int64_t const wall_time =
      std::chrono::nanoseconds{std::chrono::system_clock::now().time_since_epoch()}.count();
    uint64_t const end = rdtsc();

    if (QUILL_LIKELY(end - beg <= 2000))
    {
      _base_time = wall_time;
      _base_tsc = end;
      _resync_interval_ticks = _resync_interval_original;
      return;
    }
  }

  // we failed to return earlier and we never resynced but we don't really want to keep retrying on each call
  // to time_since_epoch() so we do non accurate resync we will increace the resynce duration to resync later
  _resync_interval_ticks = _resync_interval_ticks * 2;
}
} // namespace detail
} // namespace quill