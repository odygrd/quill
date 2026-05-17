/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"

#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/std.h"

#include <complex>
#include <cstddef>
#include <cstdint>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename T>
struct Codec<std::complex<T>>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::complex<T> const& arg) noexcept
  {
    size_t total_size = Codec<T>::compute_encoded_size(conditional_arg_size_cache, arg.real());
    total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, arg.imag());
    return total_size;
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::complex<T> const& arg) noexcept
  {
    Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.real());
    Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.imag());
  }

  static auto decode_arg(std::byte*& buffer)
  {
    using ReturnType = decltype(Codec<T>::decode_arg(buffer));

    // Brace initialization preserves left-to-right evaluation of the decode calls.
    return std::complex<ReturnType>{Codec<T>::decode_arg(buffer), Codec<T>::decode_arg(buffer)};
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
