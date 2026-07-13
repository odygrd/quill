/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/Filesystem.h"
#include "quill/core/InlinedVector.h"

#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/std.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#if defined(_WIN32)
  #include "quill/std/WideString.h"

  #include <cstring>
  #include <utility>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <>
struct Codec<fs::path>
{
  // native() returns the internal string by const reference, avoiding a temporary string

  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache, fs::path const& arg)
  {
    return Codec<fs::path::string_type>::compute_encoded_size(conditional_arg_size_cache, arg.native());
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, fs::path const& arg)
  {
    Codec<fs::path::string_type>::encode(buffer, conditional_arg_size_cache,
                                         conditional_arg_size_cache_index, arg.native());
  }

  static fs::path decode_arg(std::byte*& buffer)
  {
    using NativeStringView = std::basic_string_view<fs::path::value_type>;
    NativeStringView const native_path_view = Codec<NativeStringView>::decode_arg(buffer);

#if defined(_WIN32)
    // Encoded arguments are byte-packed and may not satisfy wchar_t alignment.
    fs::path::string_type native_path(native_path_view.size(), fs::path::value_type{});
    if (!native_path_view.empty())
    {
      std::memcpy(native_path.data(), native_path_view.data(),
                  native_path_view.size() * sizeof(fs::path::value_type));
    }
    return fs::path{std::move(native_path)};
#else
    return fs::path{native_path_view};
#endif
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
