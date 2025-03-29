/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/ChronoTimeUtils.h"
#include "quill/core/Common.h"
#include "quill/core/Rdtsc.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * Converts tsc ticks to nanoseconds since epoch
 */
class RdtscClock
{
public:
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
    RdtscTicks()
    {
      // Convert rdtsc to wall time.
      // 1. Get real time and rdtsc current count
      // 2. Calculate how many rdtsc ticks can occur in one
      // calculate _ticks_per_ns as the median over a number of observations
      // we use always odd number of trials for easy median calc
      constexpr std::chrono::milliseconds spin_duration = std::chrono::milliseconds{10};
      constexpr size_t max_trials = 15;
      constexpr size_t min_trials = 3;
      constexpr double convergence_threshold = 0.01; // 1% threshold

      std::vector<double> rates;
      rates.reserve(max_trials);

      double previous_median = 0.0;

      for (size_t i = 0; i < max_trials; ++i)
      {
        auto const beg_ts = detail::get_timestamp<std::chrono::steady_clock>();
        uint64_t const beg_tsc = rdtsc();
        uint64_t end_tsc;
        std::chrono::nanoseconds elapsed_ns;

        do
        {
          auto const end_ts = detail::get_timestamp<std::chrono::steady_clock>();
          end_tsc = rdtsc();
          elapsed_ns = end_ts - beg_ts;
        } while (elapsed_ns < spin_duration);

        rates.push_back(static_cast<double>(end_tsc - beg_tsc) / static_cast<double>(elapsed_ns.count()));

        // Check for convergence after minimum trials and only on an odd count of trials.
        if (((i + 1) >= min_trials) && (((i + 1) % 2) != 0))
        {
          std::nth_element(rates.begin(), rates.begin() + static_cast<ptrdiff_t>((i + 1) / 2), rates.end());
          double current_median = rates[(i + 1) / 2];

          // If we've converged, break early
          if (std::abs(current_median - previous_median) / current_median < convergence_threshold)
          {
            break;
          }

          previous_median = current_median;
        }
      }

      // Calculate final median.
      std::nth_element(rates.begin(), rates.begin() + static_cast<ptrdiff_t>(rates.size() / 2),
                       rates.end());

      double const ticks_per_ns = rates[rates.size() / 2];
      _ns_per_tick = 1 / ticks_per_ns;
    }

    double _ns_per_tick{0};
  };

  /***/
  explicit RdtscClock(std::chrono::nanoseconds resync_interval)
    : _resync_interval_ticks(static_cast<std::int64_t>(static_cast<double>(resync_interval.count()) *
                                                       RdtscTicks::instance().ns_per_tick())),
      _resync_interval_original(_resync_interval_ticks),
      _ns_per_tick(RdtscTicks::instance().ns_per_tick())

  {
    if (!resync(2500))
    {
      // try to resync again with higher lag
      if (!resync(10000))
      {
        std::fprintf(stderr, "Failed to sync RdtscClock. Timestamps will be incorrect\n");
      }
    }
  }

  /***/
  uint64_t time_since_epoch(uint64_t rdtsc_value) const noexcept
  {
    // should only get called by the backend thread

    // get the current index, this is only sef called my the thread that is doing the resync
    auto const index = _version.load(std::memory_order_relaxed) & (_base.size() - 1);

    // get rdtsc current value and compare the diff then add it to base wall time
    auto diff = static_cast<int64_t>(rdtsc_value - _base[index].base_tsc);

    // we need to sync after we calculated otherwise base_tsc value will be ahead of passed tsc value
    if (diff > _resync_interval_ticks)
    {
      resync(2500);
      diff = static_cast<int64_t>(rdtsc_value - _base[index].base_tsc);
    }

    return static_cast<uint64_t>(_base[index].base_time +
                                 static_cast<int64_t>(static_cast<double>(diff) * _ns_per_tick));
  }

  /***/
  uint64_t time_since_epoch_safe(uint64_t rdtsc_value) const noexcept
  {
    // thread-safe, can be called by anyone
    // this function won't resync as it can be called by anyone and only a single thread resyncs
    uint32_t version;
    uint64_t wall_ts;

    do
    {
      version = _version.load(std::memory_order_acquire);
      auto const index = version & (_base.size() - 1);

      if (QUILL_UNLIKELY((_base[index].base_tsc) == 0 && (_base[index].base_time == 0)))
      {
        return 0;
      }

      // get rdtsc current value and compare the diff then add it to base wall time
      auto const diff = static_cast<int64_t>(rdtsc_value - _base[index].base_tsc);
      wall_ts = static_cast<uint64_t>(_base[index].base_time +
                                      static_cast<int64_t>(static_cast<double>(diff) * _ns_per_tick));
    } while (version != _version.load(std::memory_order_acquire));

    return wall_ts;
  }

  /***/
  bool resync(uint32_t lag) const noexcept
  {
    // Sometimes we might get an interrupt and might never resync, so we will try again up to max_attempts
    constexpr uint8_t max_attempts{4};

    for (uint8_t attempt = 0; attempt < max_attempts; ++attempt)
    {
      uint64_t const beg = rdtsc();
      // we force convert to nanoseconds because the precision of system_clock::time-point is not portable across platforms.
      auto const wall_time = static_cast<int64_t>(detail::get_timestamp_ns<std::chrono::system_clock>());
      uint64_t const end = rdtsc();

      if (QUILL_LIKELY(end - beg <= lag))
      {
        // update the next index
        auto const index = (_version.load(std::memory_order_relaxed) + 1) & (_base.size() - 1);
        _base[index].base_time = wall_time;
        _base[index].base_tsc = _fast_average(beg, end);
        _version.fetch_add(1, std::memory_order_release);

        _resync_interval_ticks = _resync_interval_original;
        return true;
      }
    }

    // we failed to return earlier and we never resynced, but we don't really want to keep retrying on each call
    // to time_since_epoch() so we do non accurate resync we will increase the resync duration to resync later
    _resync_interval_ticks = _resync_interval_ticks * 2;
    return false;
  }

  /***/
  double nanoseconds_per_tick() const noexcept { return _ns_per_tick; }

private:
  struct BaseTimeTsc
  {
    BaseTimeTsc() = default;
    int64_t base_time{0}; /**< Get the initial base time in nanoseconds from epoch */
    uint64_t base_tsc{0}; /**< Get the initial base tsc time */
  };

  /***/
  QUILL_NODISCARD static uint64_t _fast_average(uint64_t x, uint64_t y) noexcept
  {
    return (x & y) + ((x ^ y) >> 1);
  }

private:
  mutable int64_t _resync_interval_ticks{0};
  int64_t _resync_interval_original{0}; /**< stores the initial interval value as as if we fail to resync we increase the timer */
  double _ns_per_tick{0};

  alignas(QUILL_CACHE_LINE_ALIGNED) mutable std::atomic<uint32_t> _version{0};
  mutable std::array<BaseTimeTsc, 2> _base{};
};
} // namespace detail

QUILL_END_NAMESPACE