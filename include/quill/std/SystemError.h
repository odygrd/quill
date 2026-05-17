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

#include <cstddef>
#include <cstdint>
#include <system_error>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <>
struct Codec<std::error_code>
{
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::error_code const& arg) noexcept
  {
    size_t total_size = Codec<int>::compute_encoded_size(conditional_arg_size_cache, arg.value());
    total_size += Codec<uintptr_t>::compute_encoded_size(
      conditional_arg_size_cache, reinterpret_cast<uintptr_t>(&arg.category()));
    return total_size;
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::error_code const& arg) noexcept
  {
    Codec<int>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.value());
    Codec<uintptr_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                             reinterpret_cast<uintptr_t>(&arg.category()));
  }

  static std::error_code decode_arg(std::byte*& buffer)
  {
    int const value = Codec<int>::decode_arg(buffer);
    auto const* category = reinterpret_cast<std::error_category const*>(Codec<uintptr_t>::decode_arg(buffer));
    QUILL_ASSERT(category, "Codec<std::error_code>::decode_arg() decoded a null error category");
    return std::error_code{value, *category};
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};
QUILL_END_EXPORT

QUILL_END_NAMESPACE
