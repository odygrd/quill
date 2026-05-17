/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/DeferredFormatCodec.h"
#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"

#include "quill/bundled/fmt/chrono.h"
#include "quill/bundled/fmt/format.h"

#include <chrono>
#include <ctime>
#include <type_traits>

// Enable codecs for the C++20 calendar chrono types that bundled fmt formats
// natively: std::chrono::year, month, day, weekday and year_month_day.
//
// We strictly mirror fmt's own gate here: `fmtquill::{year,month,day,weekday,
// year_month_day}` only alias the `std::chrono::*` types (and therefore only
// have formatters reachable via `std::chrono::*`) when `__cpp_lib_chrono >=
// 201907L`. On older standard libraries the fmtquill types are distinct
// fallback classes, so formatting a `std::chrono::year` would fail to compile.
// Because of that we do not expose a manual override — the gate is either on
// because the standard library is advertised as conforming, or it is off.
#if defined(__cpp_lib_chrono) && (__cpp_lib_chrono >= 201907L)
  #define QUILL_HAS_CXX20_CHRONO 1
#else
  #define QUILL_HAS_CXX20_CHRONO 0
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

// All of the chrono types we cover here are trivially copyable and default
// constructible, so DeferredFormatCodec<T> uses its zero-overhead memcpy
// branch on both the frontend and backend — exactly what we need for a low
// latency logging hot path.
template <template <typename...> class ChronoType, typename TimeSpec, typename DurationType>
struct Codec<ChronoType<TimeSpec, DurationType>,
             std::enable_if_t<std::disjunction_v<std::is_same<ChronoType<TimeSpec, DurationType>, std::chrono::time_point<TimeSpec, DurationType>>,
                                                 std::is_same<ChronoType<TimeSpec, DurationType>, std::chrono::duration<TimeSpec, DurationType>>>>>
  : DeferredFormatCodec<ChronoType<TimeSpec, DurationType>>
{
};

template <>
struct Codec<std::tm> : DeferredFormatCodec<std::tm>
{
};

#if QUILL_HAS_CXX20_CHRONO
template <>
struct Codec<std::chrono::year> : DeferredFormatCodec<std::chrono::year>
{
};

template <>
struct Codec<std::chrono::month> : DeferredFormatCodec<std::chrono::month>
{
};

template <>
struct Codec<std::chrono::day> : DeferredFormatCodec<std::chrono::day>
{
};

template <>
struct Codec<std::chrono::weekday> : DeferredFormatCodec<std::chrono::weekday>
{
};

template <>
struct Codec<std::chrono::year_month_day> : DeferredFormatCodec<std::chrono::year_month_day>
{
};
#endif

QUILL_END_EXPORT

QUILL_END_NAMESPACE
