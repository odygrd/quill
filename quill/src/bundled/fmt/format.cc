// Formatting library for C++
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "quill/bundled/fmt/format-inl.h"

FMTQUILL_BEGIN_NAMESPACE
namespace detail {

template FMTQUILL_API auto dragonbox::to_decimal(float x) noexcept
    -> dragonbox::decimal_fp<float>;
template FMTQUILL_API auto dragonbox::to_decimal(double x) noexcept
    -> dragonbox::decimal_fp<double>;

#ifndef FMTQUILL_STATIC_THOUSANDS_SEPARATOR
template FMTQUILL_API locale_ref::locale_ref(const std::locale& loc);
template FMTQUILL_API auto locale_ref::get<std::locale>() const -> std::locale;
#endif

// Explicit instantiations for char.

template FMTQUILL_API auto thousands_sep_impl(locale_ref)
    -> thousands_sep_result<char>;
template FMTQUILL_API auto decimal_point_impl(locale_ref) -> char;

template FMTQUILL_API void buffer<char>::append(const char*, const char*);

template FMTQUILL_API void vformat_to(buffer<char>&, string_view,
                                 typename vformat_args<>::type, locale_ref);

// Explicit instantiations for wchar_t.

template FMTQUILL_API auto thousands_sep_impl(locale_ref)
    -> thousands_sep_result<wchar_t>;
template FMTQUILL_API auto decimal_point_impl(locale_ref) -> wchar_t;

template FMTQUILL_API void buffer<wchar_t>::append(const wchar_t*, const wchar_t*);

}  // namespace detail
FMTQUILL_END_NAMESPACE
