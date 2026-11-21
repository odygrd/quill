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
  #include <bitset>
  #include <cstddef>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * Copies the bitset to the backend, which formats it. Standard libraries make std::bitset
 * trivially copyable, so this is a single memcpy on the calling thread.
 */
template <size_t N>
struct Codec<std::bitset<N>> : DeferredFormatCodec<std::bitset<N>>
{
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
