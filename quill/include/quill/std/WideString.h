/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#if defined(_WIN32)

  #include "quill/core/Codec.h"
  #include "quill/core/DynamicFormatArgStore.h"
  #include "quill/core/Utf8Conv.h"

  #include "quill/bundled/fmt/format.h"

  #include <cstddef>
  #include <cstdint>
  #include <string>
  #include <string_view>
  #include <type_traits>
  #include <vector>

namespace quill::detail
{
/***/
template <typename T>
struct ArgSizeCalculator<
  T, std::enable_if_t<std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>, std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache, T const& arg) noexcept
  {
    // Calculate the size of the string in bytes
    size_t len;

    if constexpr (std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>>)
    {
      len = wcslen(arg);
    }
    else
    {
      len = arg.size();
    }

    conditional_arg_size_cache.push_back(len);

    // also include the size of the string in the buffer as a separate variable
    // we can retrieve it when we decode. We do not store the null terminator in the buffer
    return static_cast<size_t>(sizeof(size_t) + (len * sizeof(wchar_t)));
  }
};

/***/
template <typename T>
struct Encoder<T,
               std::enable_if_t<std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                                   std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, T const& arg) noexcept
  {
    // The wide string size in bytes
    size_t const len = conditional_arg_size_cache[conditional_arg_size_cache_index++];
    std::memcpy(buffer, &len, sizeof(len));
    buffer += sizeof(len);

    if (len != 0)
    {
      // copy the string, no need to zero terminate it as we got the length and e.g a wstring_view
      // might not always be zero terminated
      size_t const size_in_bytes = len * sizeof(wchar_t);

      if constexpr (std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>>)
      {
        std::memcpy(buffer, arg, size_in_bytes);
      }
      else
      {
        std::memcpy(buffer, arg.data(), size_in_bytes);
      }

      buffer += size_in_bytes;
    }
  }
};

/***/
template <typename T>
struct Decoder<T,
               std::enable_if_t<std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                                   std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>>>
{
  static auto decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    // we first need to retrieve the length
    size_t len;
    std::memcpy(&len, buffer, sizeof(len));
    buffer += sizeof(len);

    std::wstring_view wstr{reinterpret_cast<wchar_t const*>(buffer), len};

    if (args_store)
    {
      std::string str = utf8_encode(buffer, len);
      args_store->push_back(static_cast<std::string&&>(str));
    }

    size_t size_bytes = len * sizeof(wchar_t);
    buffer += size_bytes;
    return wstr;
  }
};
} // namespace quill::detail
#endif