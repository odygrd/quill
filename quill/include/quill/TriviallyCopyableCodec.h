/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include <cstring>
#include <type_traits>

/**
 * @brief Defines serialization (codec) functionality for trivially copyable userdefined types.
 *
 * This macro provides specializations for the `ArgSizeCalculator`, `Encoder`, and `Decoder`
 * templates in the `quill` namespace. It is designed for types that are trivially copyable,
 * meaning they can be copied with a simple `memcpy` operation.
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
 * QUILL_DEFINE_TRIVIALLY_COPYABLE_CODEC(TCStruct);
 *
 * int main()
 * {
 *   // init code ..
 *   TCStruct tc;
 *   LOG_INFO(logger, "{}", tc);
 * }
 * \endcode
 */

#define QUILL_DEFINE_TRIVIALLY_COPYABLE_CODEC(Arg)                                                           \
                                                                                                             \
  static_assert(std::is_trivially_copyable_v<Arg>,                                                           \
                "Arg must be trivially copyable for this serialization macro. Non-trivially "                \
                "copyable types can still be logged, but you will need a different approach. "               \
                "Please refer to the documentation or examples for alternative methods.");                   \
                                                                                                             \
  template <>                                                                                                \
  struct quill::ArgSizeCalculator<Arg>                                                                       \
  {                                                                                                          \
    static size_t calculate(std::vector<size_t>&, ::Arg const& arg) noexcept                                 \
    {                                                                                                        \
      return sizeof(arg);                                                                                    \
    }                                                                                                        \
  };                                                                                                         \
                                                                                                             \
  template <>                                                                                                \
  struct quill::Encoder<Arg>                                                                                 \
  {                                                                                                          \
    static void encode(std::byte*& buffer, std::vector<size_t> const&, uint32_t&, ::Arg const& arg) noexcept \
    {                                                                                                        \
      std::memcpy(buffer, &arg, sizeof(arg));                                                                \
      buffer += sizeof(arg);                                                                                 \
    }                                                                                                        \
  };                                                                                                         \
                                                                                                             \
  template <>                                                                                                \
                                                                                                             \
  struct quill::Decoder<Arg>                                                                                 \
  {                                                                                                          \
    static ::Arg decode(std::byte*& buffer, DynamicFormatArgStore* args_store)                               \
    {                                                                                                        \
      ::Arg arg;                                                                                             \
      std::memcpy(&arg, buffer, sizeof(arg));                                                                \
      buffer += sizeof(arg);                                                                                 \
      if (args_store)                                                                                        \
      {                                                                                                      \
        args_store->push_back(arg);                                                                          \
      }                                                                                                      \
      return arg;                                                                                            \
    }                                                                                                        \
  };