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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename T>
struct Codec<std::atomic<T>>
{
  static_assert(std::is_arithmetic_v<T>, "Atomic logging supports arithmetic types only");

  static size_t compute_encoded_size(detail::SizeCacheVector&, std::atomic<T> const&) noexcept
  {
    return sizeof(T);
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::atomic<T> const& arg) noexcept
  {
    Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                     arg.load(std::memory_order_relaxed));
  }

  static auto decode_arg(std::byte*& buffer) { return Codec<T>::decode_arg(buffer); }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
