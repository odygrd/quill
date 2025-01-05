/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstdint>

#include "quill/TriviallyCopyableCodec.h"
#include "quill/backend/RdtscClock.h"
#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/Rdtsc.h"

#include "quill/bundled/fmt/format.h"
#include "quill/std/Chrono.h"

QUILL_BEGIN_NAMESPACE

namespace detail
{
/**
 * A stopwatch utility for measuring elapsed time since construction.
 *
 * Displays elapsed time as seconds (double) or any specified duration type.
 *
 * Can use either TSC-based or system clock-based timing, depending on the
 * template parameter `ClockType`.
 *
 * Example:
 * @code
 *   quill::StopWatchTsc swt;
 *   LOG_INFO(logger, "Begin");
 *   std::this_thread::sleep_for(std::chrono::seconds(1));
 *   LOG_INFO(logger, "After 1s, elapsed: {:.6}", swt); // => elapsed: ~1.000000
 *   std::this_thread::sleep_for(std::chrono::milliseconds(500));
 *   LOG_INFO(logger, "After 500ms, elapsed: {}", swt); // => elapsed: ~1.500000
 * @endcode
 */
template <ClockSourceType ClockType>
class StopWatch
{
public:
  static_assert((ClockType == ClockSourceType::Tsc) || (ClockType == ClockSourceType::System),
                "Invalid ClockType");

  /**
   * Constructor
   */
  StopWatch()
  {
    if constexpr (ClockType == ClockSourceType::Tsc)
    {
      _ns_per_tick = RdtscClock::RdtscTicks::instance().ns_per_tick();
      _start_tp = rdtsc();
    }
    else
    {
      _start_tp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
    }
  }

  /**
   * Returns the elapsed time since construction.
   * @return The elapsed time as a `std::chrono::duration<double>` in seconds.
   */
  [[nodiscard]] std::chrono::duration<double> elapsed() const
  {
    if constexpr (ClockType == ClockSourceType::Tsc)
    {
      return std::chrono::duration<double>(
        std::chrono::nanoseconds{static_cast<uint64_t>((rdtsc() - _start_tp) * _ns_per_tick)});
    }
    else
    {
      return std::chrono::duration<double>(std::chrono::nanoseconds{
        std::chrono::steady_clock::now().time_since_epoch().count() - _start_tp});
    }
  }

  /**
   * Returns the elapsed time since construction as the specified duration type.
   * @return The elapsed time converted to the specified duration type.
   */
  template <typename T>
  [[nodiscard]] T elapsed_as() const
  {
    if constexpr (ClockType == ClockSourceType::Tsc)
    {
      return std::chrono::duration_cast<T>(
        std::chrono::nanoseconds{static_cast<uint64_t>((rdtsc() - _start_tp) * _ns_per_tick)});
    }
    else
    {
      return std::chrono::duration_cast<T>(std::chrono::nanoseconds{
        std::chrono::steady_clock::now().time_since_epoch().count() - _start_tp});
    }
  }

  /**
   * Resets the stopwatch, starting the measurement from the current time.
   */
  void reset()
  {
    if constexpr (ClockType == ClockSourceType::Tsc)
    {
      _start_tp = rdtsc();
    }
    else
    {
      _start_tp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
    }
  }

private:
  double _ns_per_tick{0};
  uint64_t _start_tp{0};
};
} // namespace detail

/**
 * Stopwatch using TSC (Time Stamp Counter) for high-resolution timing.
 */
using StopWatchTsc = detail::StopWatch<ClockSourceType::Tsc>;

/**
 * Stopwatch using the system clock for timing based on `std::chrono`.
 */
using StopWatchChrono = detail::StopWatch<ClockSourceType::System>;

QUILL_END_NAMESPACE

template <quill::ClockSourceType ClockType>
struct quill::Codec<quill::detail::StopWatch<ClockType>>
  : quill::TriviallyCopyableTypeCodec<quill::detail::StopWatch<ClockType>>
{
};

template <quill::ClockSourceType ClockType>
struct fmtquill::formatter<quill::detail::StopWatch<ClockType>> : fmtquill::formatter<double>
{
  template <typename FormatContext>
  auto format(quill::detail::StopWatch<ClockType> const& sw, FormatContext& ctx) const
    -> decltype(ctx.out())
  {
    return fmtquill::formatter<double>::format(sw.elapsed().count(), ctx);
  }
};
