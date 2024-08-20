/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/DynamicFormatArgStore.h"

#include <cstring>
#include <type_traits>

QUILL_BEGIN_NAMESPACE

/**
 * @brief Defines serialization (codec) functionality for trivially copyable user defined types.
 *
 * Example usage:
 *
 * \code{.cpp}
 * struct TCStruct
 * {
 *   int a;
 *   double b;
 *   char c[12];
 *
 *   friend std::ostream& operator<<(std::ostream& os, TCStruct const& arg)
 *   {
 *     os << "a: " << arg.a << ", b: " << arg.b << ", c: " << arg.c;
 *     return os;
 *   }
 * };
 *
 * template <>
 * struct fmtquill::formatter<TCStruct> : fmtquill::ostream_formatter
 * {
 * };
 *
 * template <>
 * struct quill::Codec<TCStruct> : TriviallyCopyableTypeCodec<TCStruct>
 * {
 * };
 *
 * int main()
 * {
 *   // init code ..
 *   TCStruct tc;
 *   LOG_INFO(logger, "{}", tc);
 * }
 * \endcode
 */

template <typename T>
struct TriviallyCopyableTypeCodec
{
  static_assert(std::is_trivially_copyable_v<T>,
                "T must be trivially copyable. Non-trivially copyable types can still be logged, "
                "but you will need a different approach. Please refer to the documentation or "
                "examples for alternative methods.");

  static size_t compute_encoded_size(detail::SizeCacheVector&, T const& arg) noexcept
  {
    return sizeof(arg);
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const&, uint32_t&, T const& arg) noexcept
  {
    std::memcpy(buffer, &arg, sizeof(arg));
    buffer += sizeof(arg);
  }

  static T decode_arg(std::byte*& buffer)
  {
    T arg;

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