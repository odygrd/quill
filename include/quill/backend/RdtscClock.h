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
#include "quill/core/ThreadPrimitives.h"

#ifndef QUILL_MODULE
  #include <array>
  #include <atomic>
  #include <chrono>
  #include <cstdint>
  #include <cstdio>
  #include <limits>
  #include <utility>
#endif

QUILL_BEGIN_NAMESPACE

namespace detail
{

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(push)
  #pragma warning(disable : 4324)
#endif

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
    QUILL_NODISCARD QUILL_EXPORT static inline RdtscTicks& instance()
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
#if defined(__aarch64__)
      // On AArch64 the generic timer frequency is exposed directly
      uint64_t freq;
      __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));
      // freq is typically exactly 24000000, 50000000, or 1000000000 depending on the SoC
      _ns_per_tick = 1e9 / static_cast<double>(freq);
#else
      // Measure the tick rate against the steady clock between two points 10 ms apart. Each point
      // takes the tightest of several rdtsc/clock/rdtsc reads, so preemption between the points
      // does not affect the result; RdtscClock refines the rate at each resync.
      auto const sample = []()
      {
        constexpr uint32_t attempts{8};
        uint64_t best_lag{(std::numeric_limits<uint64_t>::max)()};
        std::pair<uint64_t, uint64_t> best_sample{};

        for (uint32_t attempt = 0; attempt < attempts; ++attempt)
        {
          uint64_t const beg_tsc = rdtsc();
          uint64_t const steady_ns = detail::get_steady_time_ns();
          uint64_t const end_tsc = rdtsc();

          if ((end_tsc - beg_tsc) < best_lag)
          {
            best_lag = end_tsc - beg_tsc;
            best_sample = {steady_ns, beg_tsc + ((end_tsc - beg_tsc) / 2u)};
          }
        }

        return best_sample;
      };

      constexpr uint64_t calibration_interval_ns{10'000'000};

      auto const begin = sample();
      detail::sleep_for_ns(calibration_interval_ns);
      auto end = sample();

      while ((end.first - begin.first) < calibration_interval_ns)
      {
        // a sleep can wake up early on some platforms
        end = sample();
      }

      _ns_per_tick =
        static_cast<double>(end.first - begin.first) / static_cast<double>(end.second - begin.second);
#endif
    }

    double _ns_per_tick{0};
  };

  /***/
  explicit RdtscClock(std::chrono::nanoseconds resync_interval)
    : _ns_per_tick(RdtscTicks::instance().ns_per_tick())
  {
    reconfigure(resync_interval);
  }

  /**
   * Reconfigures the resync interval and publishes a fresh clock snapshot.
   * This is a single-backend-writer operation and may run concurrently with
   * time_since_epoch_safe().
   */
  void reconfigure(std::chrono::nanoseconds resync_interval) noexcept
  {
    double const calc_value = static_cast<double>(resync_interval.count()) / _ns_per_tick;

    // Check for overflow and negative values
    if (calc_value >= static_cast<double>((std::numeric_limits<int64_t>::max)()) || calc_value < 0)
    {
      _resync_interval_ticks = (std::numeric_limits<int64_t>::max)();
    }
    else
    {
      _resync_interval_ticks = static_cast<int64_t>(calc_value);
    }

    _resync_interval_original = _resync_interval_ticks;

    if (!resync(resync_lag_cycles))
    {
      // try to resync again with higher lag
      if (!resync(resync_lag_cycles * 2u))
      {
        std::fprintf(stderr, "Failed to sync RdtscClock. Timestamps will be incorrect\n");
      }
    }
  }

  /***/
  uint64_t time_since_epoch(uint64_t rdtsc_value) const noexcept
  {
    // should only get called by the backend thread

    // get the current index, this is only safe to call from the thread that is doing the resync
    auto index = _version.load(std::memory_order_relaxed) & (_base.size() - 1);

    // Unsigned subtraction + int64_t cast: a stale rdtsc_value yields a small negative
    // diff, producing a wall time slightly in the past. This is intentional.
    auto const diff =
      static_cast<int64_t>(rdtsc_value - _base[index].base_tsc.load(std::memory_order_relaxed));

    // we need to sync after we calculated otherwise base_tsc value will be ahead of passed tsc value
    if (diff > _resync_interval_ticks)
    {
      resync(resync_lag_cycles);
      index = _version.load(std::memory_order_relaxed) & (_base.size() - 1);
    }

    return _convert(_base[index], rdtsc_value);
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
      if (QUILL_UNLIKELY((_base[index].base_tsc.load(std::memory_order_relaxed) == 0) &&
                         (_base[index].base_time.load(std::memory_order_relaxed) == 0)))
      {
        return 0;
      }

      wall_ts = _convert(_base[index], rdtsc_value);

      // The fence prevents the relaxed data loads above from being reordered after the version
      // re-check below on weakly-ordered architectures; an acquire load alone does not stop
      // earlier loads from sinking past it
      std::atomic_thread_fence(std::memory_order_acquire);
    } while (version != _version.load(std::memory_order_relaxed));

    return wall_ts;
  }

  /**
   * Resyncs from the logging path. On failure the resync interval is doubled so that
   * time_since_epoch() does not retry on every call.
   */
  bool resync(uint32_t lag) const noexcept
  {
    if (try_resync(lag))
    {
      _resync_interval_ticks = _resync_interval_original;
      return true;
    }

    constexpr int64_t max_int64_half = (std::numeric_limits<int64_t>::max)() / 2;
    if (_resync_interval_ticks <= max_int64_half)
    {
      _resync_interval_ticks = _resync_interval_ticks * 2;
    }

    return false;
  }

  /**
   * Resyncs while the backend is idle. Prefers a tight lag for an accurate base, and falls back to
   * the logging path's budget so a slow system clock cannot leave the clock unsynchronized. Unlike
   * resync(), a failure here does not lengthen the logging path's resync interval.
   */
  bool resync_when_idle() const noexcept
  {
    constexpr uint32_t idle_resync_lag_cycles{2'500};
    return try_resync(idle_resync_lag_cycles) || try_resync(resync_lag_cycles);
  }

  /**
   * Publishes a new base if the system clock can be read within `lag` ticks.
   * @return true on success. Leaves the resync interval unchanged.
   */
  bool try_resync(uint32_t lag) const noexcept
  {
    // Sometimes we might get an interrupt and might never resync, so we will try again up to max_attempts
    constexpr uint8_t max_attempts{4};

    for (uint8_t attempt = 0; attempt < max_attempts; ++attempt)
    {
      uint64_t const beg = rdtsc();
      // we force convert to nanoseconds because the precision of system_clock::time-point is not portable across platforms.
      auto const wall_time = static_cast<int64_t>(detail::get_system_time_ns());
      uint64_t const end = rdtsc();

      if (QUILL_LIKELY(end - beg <= lag))
      {
        uint64_t const tsc = _fast_average(beg, end);
        BaseTimeTsc const& current = _base[_version.load(std::memory_order_relaxed) & (_base.size() - 1)];
        double const ns_per_tick = _refined_ns_per_tick(current, wall_time, tsc);

        // update the next index
        auto const index = (_version.load(std::memory_order_relaxed) + 1) & (_base.size() - 1);
        _base[index].base_time.store(wall_time, std::memory_order_relaxed);
        _base[index].base_tsc.store(tsc, std::memory_order_relaxed);
        _base[index].ns_per_tick.store(ns_per_tick, std::memory_order_relaxed);
        _version.fetch_add(1, std::memory_order_release);
        return true;
      }
    }

    return false;
  }

  /** The tick rate measured at startup, before any refinement */
  double nanoseconds_per_tick() const noexcept { return _ns_per_tick; }

protected:
  struct BaseTimeTsc
  {
    BaseTimeTsc() = default;
    std::atomic<int64_t> base_time{0}; /**< Get the initial base time in nanoseconds from epoch */
    std::atomic<uint64_t> base_tsc{0}; /**< Get the initial base tsc time */
    std::atomic<double> ns_per_tick{0}; /**< Tick rate used from this base */
  };

  /** Converts a tick value using a published base; the caller ensures the base is consistent */
  QUILL_NODISCARD static uint64_t _convert(BaseTimeTsc const& base, uint64_t rdtsc_value) noexcept
  {
    auto const diff = static_cast<int64_t>(rdtsc_value - base.base_tsc.load(std::memory_order_relaxed));
    return static_cast<uint64_t>(
      base.base_time.load(std::memory_order_relaxed) +
      static_cast<int64_t>(static_cast<double>(diff) * base.ns_per_tick.load(std::memory_order_relaxed)));
  }

  /**
   * The startup rate is measured over a few milliseconds and the wall clock is NTP-disciplined,
   * so a fixed rate drifts from the wall clock and each resync steps the time, possibly backwards.
   * Re-derive the rate from the previous base. Keep the previous rate when the interval is too
   * short to be accurate, or when the result is implausible, e.g. after the wall clock was stepped.
   * Without a previous base to measure from, use the startup rate.
   */
  QUILL_NODISCARD double _refined_ns_per_tick(BaseTimeTsc const& previous, int64_t wall_time, uint64_t tsc) const noexcept
  {
    double const previous_ns_per_tick = previous.ns_per_tick.load(std::memory_order_relaxed);
    uint64_t const previous_tsc = previous.base_tsc.load(std::memory_order_relaxed);
    int64_t const previous_time = previous.base_time.load(std::memory_order_relaxed);

    if (!(previous_ns_per_tick > 0.0) || (tsc <= previous_tsc))
    {
      return _ns_per_tick;
    }

    constexpr double min_refinement_interval_ns{100'000'000}; // 100 ms
    constexpr double max_deviation{500e-6};                   // 500 ppm from the startup rate

    double const elapsed_ticks = static_cast<double>(tsc - previous_tsc);

    if ((elapsed_ticks * _ns_per_tick) < min_refinement_interval_ns)
    {
      return previous_ns_per_tick;
    }

    double const measured = static_cast<double>(wall_time - previous_time) / elapsed_ticks;
    double const deviation = (measured / _ns_per_tick) - 1.0;

    return ((deviation > max_deviation) || (deviation < -max_deviation)) ? previous_ns_per_tick : measured;
  }

  /***/
  QUILL_NODISCARD static uint64_t _fast_average(uint64_t x, uint64_t y) noexcept
  {
    return (x & y) + ((x ^ y) >> 1);
  }

protected:
  static constexpr uint32_t resync_lag_cycles{50'000};
  mutable int64_t _resync_interval_ticks{0};
  int64_t _resync_interval_original{0}; /**< stores the initial interval value as as if we fail to resync we increase the timer */
  double _ns_per_tick{0};

  alignas(QUILL_CACHE_LINE_ALIGNED) mutable std::atomic<uint32_t> _version{0};
  mutable std::array<BaseTimeTsc, 2> _base{};
};

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__GNUC__)
  #pragma warning(pop)
#endif

} // namespace detail

QUILL_END_NAMESPACE
