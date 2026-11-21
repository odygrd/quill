/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/DeferredFormatCodec.h"
#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"

#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/std.h"

#ifndef QUILL_MODULE
  #include <complex>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/** std::complex is copied whole and formatted on the backend */
template <typename T>
struct Codec<std::complex<T>> : DeferredFormatCodec<std::complex<T>>
{
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
