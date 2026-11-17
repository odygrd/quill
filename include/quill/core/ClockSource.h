/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"

#ifndef QUILL_MODULE
  #include <memory>
  #include <type_traits>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

class UserClockSource;

/**
 * Preserves the clock's type so logger factories can reject objects incompatible with
 * FrontendOptions::clock_type at compile time, before storing a UserClockSource pointer.
 * Returned by ClockSource::custom(); does not own or copy the clock object.
 */
template <typename TClock>
class UserClockRef
{
public:
  explicit constexpr UserClockRef(TClock& clock) noexcept : _clock(std::addressof(clock)) {}

  template <typename TOtherClock, std::enable_if_t<std::is_convertible_v<TOtherClock*, TClock*>, int> = 0>
  constexpr UserClockRef(UserClockRef<TOtherClock> clock) noexcept : _clock(clock.get())
  {
  }

  QUILL_NODISCARD constexpr TClock* get() const noexcept { return _clock; }
  QUILL_NODISCARD constexpr ClockSourceType source_type() const noexcept
  {
    return ClockSourceType::User;
  }

private:
  TClock* _clock;
};

/** Complete per-logger clock configuration. The default is the built-in TSC clock. */
class ClockSource
{
public:
  constexpr ClockSource() noexcept = default;

  /** Select the built-in TSC clock. */
  QUILL_NODISCARD static constexpr ClockSource tsc() noexcept
  {
    return ClockSource{ClockSourceType::Tsc};
  }

  /** Select the built-in system clock. */
  QUILL_NODISCARD static constexpr ClockSource system() noexcept
  {
    return ClockSource{ClockSourceType::System};
  }

  /** Refer to an application-owned clock; it must outlive its loggers and queued records. */
  template <typename TClock>
  QUILL_NODISCARD static constexpr UserClockRef<TClock> custom(TClock& clock) noexcept
  {
    static_assert(std::is_base_of_v<UserClockSource, TClock>,
                  "ClockSource::custom() requires a UserClockSource object");

    return UserClockRef<TClock>{clock};
  }

  /** Runtime dispatch accepts any user-clock implementation. */
  template <typename TClock>
  constexpr ClockSource(UserClockRef<TClock> clock) noexcept
    : _clock(clock.get()), _source_type(ClockSourceType::User)
  {
  }

  QUILL_NODISCARD constexpr UserClockSource* get() const noexcept { return _clock; }
  QUILL_NODISCARD constexpr ClockSourceType source_type() const noexcept { return _source_type; }

private:
  explicit constexpr ClockSource(ClockSourceType source_type) noexcept : _source_type(source_type)
  {
  }

  UserClockSource* _clock{nullptr};
  ClockSourceType _source_type{ClockSourceType::Tsc};
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
