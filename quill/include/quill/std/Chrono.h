/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"

#include "quill/bundled/fmt/chrono.h"
#include "quill/bundled/fmt/format.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

QUILL_BEGIN_NAMESPACE

template <typename Clock, typename Duration>
struct Codec<std::chrono::time_point<Clock, Duration>>
{
  static_assert(std::is_trivially_copyable_v<std::chrono::time_point<Clock, Duration>>,
                "std::chrono::time_point must be trivially copyable");

  static size_t compute_encoded_size(std::vector<size_t>&, std::chrono::time_point<Clock, Duration> const& arg) noexcept
  {
    return sizeof(arg);
  }

  static void encode(std::byte*& buffer, std::vector<size_t> const&, uint32_t&,
                     std::chrono::time_point<Clock, Duration> arg) noexcept
  {
    std::memcpy(buffer, &arg, sizeof(arg));
    buffer += sizeof(arg);
  }

  static std::chrono::time_point<Clock, Duration> decode_arg(std::byte*& buffer)
  {
    std::chrono::time_point<Clock, Duration> arg;

    // Cast to void* to silence compiler warning about private members
    std::memcpy(static_cast<void*>(&arg), buffer, sizeof(arg));

    buffer += sizeof(arg);
    return arg;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

template <typename Rep, typename Period>
struct Codec<std::chrono::duration<Rep, Period>>
{
  static_assert(std::is_trivially_copyable_v<std::chrono::duration<Rep, Period>>,
                "std::chrono::duration must be trivially copyable");

  static size_t compute_encoded_size(std::vector<size_t>&, std::chrono::duration<Rep, Period> const& arg) noexcept
  {
    return sizeof(arg);
  }

  static void encode(std::byte*& buffer, std::vector<size_t> const&, uint32_t&,
                     std::chrono::duration<Rep, Period> arg) noexcept
  {
    std::memcpy(buffer, &arg, sizeof(arg));
    buffer += sizeof(arg);
  }

  static std::chrono::duration<Rep, Period> decode_arg(std::byte*& buffer)
  {
    std::chrono::duration<Rep, Period> arg;

    // Cast to void* to silence compiler warning about private members
    std::memcpy(static_cast<void*>(&arg), buffer, sizeof(arg));

    buffer += sizeof(arg);
    return arg;
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_NAMESPACE