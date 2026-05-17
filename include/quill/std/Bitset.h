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

#include <bitset>
#include <cstddef>
#include <cstdint>

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <size_t N>
struct Codec<std::bitset<N>>
{
private:
  static constexpr size_t encoded_size = (N + 7u) / 8u;

public:
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::bitset<N> const& arg) noexcept
  {
    (void)conditional_arg_size_cache;
    (void)arg;
    return encoded_size;
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, std::bitset<N> const& arg) noexcept
  {
    (void)conditional_arg_size_cache;
    (void)conditional_arg_size_cache_index;

    if constexpr (N <= 64u)
    {
      // Fast path: std::bitset<N> for N <= 64 stores its bits as a single ulong/ullong.
      // to_ullong() is O(1), so we avoid N individual test()/bit-shift/mask iterations on the
      // frontend hot path. We write the packed bytes in little-endian order with a short
      // (<= 8 iteration) loop rather than memcpy'ing a partial view of &bits, so the wire format
      // is identical on little- and big-endian targets.
      unsigned long long const bits = arg.to_ullong();
      for (size_t byte_index = 0; byte_index < encoded_size; ++byte_index)
      {
        buffer[byte_index] = std::byte{static_cast<unsigned char>((bits >> (byte_index * 8u)) & 0xFFu)};
      }
    }
    else
    {
      for (size_t byte_index = 0; byte_index < encoded_size; ++byte_index)
      {
        buffer[byte_index] = std::byte{0};
      }

      for (size_t bit_index = 0; bit_index < N; ++bit_index)
      {
        if (arg.test(bit_index))
        {
          buffer[bit_index / 8u] |= std::byte{static_cast<unsigned char>(1u << (bit_index % 8u))};
        }
      }
    }

    buffer += encoded_size;
  }

  static std::bitset<N> decode_arg(std::byte*& buffer)
  {
    std::bitset<N> arg;

    if constexpr (N <= 64u)
    {
      // Matching fast path for the encoder: reconstruct the bitset from the little-endian
      // packed bytes written above.
      unsigned long long bits = 0;
      for (size_t byte_index = 0; byte_index < encoded_size; ++byte_index)
      {
        bits |= static_cast<unsigned long long>(std::to_integer<unsigned char>(buffer[byte_index]))
          << (byte_index * 8u);
      }
      arg = std::bitset<N>{bits};
    }
    else
    {
      for (size_t bit_index = 0; bit_index < N; ++bit_index)
      {
        unsigned char const encoded_byte = std::to_integer<unsigned char>(buffer[bit_index / 8u]);

        if ((encoded_byte & static_cast<unsigned char>(1u << (bit_index % 8u))) != 0)
        {
          arg.set(bit_index);
        }
      }
    }

    buffer += encoded_size;
    return arg;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
