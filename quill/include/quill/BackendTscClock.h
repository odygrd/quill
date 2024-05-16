/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/BackendManager.h"
#include "quill/core/Attributes.h"
#include "quill/core/Rdtsc.h"

#include <chrono>
#include <cstdint>

namespace quill
{
/**
 * A utility class for accessing the Time Stamp Counter (TSC) clock used by the backend logging thread.
 * This class allows other threads to obtain timestamps synchronized with the TSC clock of the backend logging thread.
 *
 * When `default_timestamp_clock_type = TimestampClockType::Tsc` is used, this class provides access to
 * the TSC clock maintained by the backend logging thread for synchronized timestamp retrieval.
 *
 * If `TimestampClockType::Tsc` is not enabled in Config.h, this class reverts to using the system clock
 * for timestamps.
 *
 * @note For more accurate timestamps, consider reducing rdtsc_resync_interval in Config.h.
 * @note All methods of the class are thread-safe.
 */
class BackendTscClock
{
public:
  class RdtscVal
  {
  public:
    RdtscVal(RdtscVal const& other) = default;
    RdtscVal(RdtscVal&& other) noexcept = default;
    RdtscVal& operator=(RdtscVal const& other) = default;
    RdtscVal& operator=(RdtscVal&& other) noexcept = default;

    QUILL_NODISCARD_ALWAYS_INLINE_HOT uint64_t value() const noexcept { return _value; }

  private:
    RdtscVal() noexcept : _value(detail::rdtsc()) {}
    friend BackendTscClock;

    uint64_t _value;
  };

public:
  using duration = std::chrono::nanoseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<BackendTscClock, duration>;
  static constexpr bool is_steady = false;

  /**
   * Provides the current synchronized timestamp obtained using the TSC clock maintained by the backend logging thread.
   * @return A wall clock timestamp in nanoseconds since epoch, synchronized with the backend logging thread's TSC clock.
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT static time_point now() noexcept
  {
    uint64_t const ts = detail::BackendManager::instance().convert_rdtsc_to_epoch_time(detail::rdtsc());

    return ts ? time_point{std::chrono::nanoseconds{ts}}
              : time_point{std::chrono::nanoseconds{
                  std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now())
                    .time_since_epoch()
                    .count()}};
  }

  /**
   * Returns the current value of the TSC timer maintained by the backend logging thread.
   * @return The current value of the TSC timer.
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT static RdtscVal rdtsc() noexcept { return RdtscVal{}; }

  /**
   * Converts a TSC counter value obtained from the backend logging thread's TSC timer to a wall clock timestamp.
   *
   * @param rdtsc The TSC counter value obtained from the backend logging thread's TSC timer.
   * @warning This function will return `0` when TimestampClockType::Tsc is not enabled in Config.h.
   * @return Time since epoch in nanoseconds.
   */
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT static time_point to_time_point(RdtscVal rdtsc) noexcept
  {
    return time_point{std::chrono::nanoseconds{
      detail::BackendManager::instance().convert_rdtsc_to_epoch_time(rdtsc.value())}};
  }
};
} // namespace quill