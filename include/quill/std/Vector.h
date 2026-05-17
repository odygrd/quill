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
#include "quill/bundled/fmt/ranges.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
  #include <string>
  #include <string_view>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

template <typename T, typename Allocator>
struct Codec<std::vector<T, Allocator>>
{
private:
  static constexpr bool use_memcpy_fast_path = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

public:
  static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache,
                                     std::vector<T, Allocator> const& arg)
  {
    // We need to store the size of the vector in the buffer, so we reserve space for it.
    // We add sizeof(size_t) bytes to accommodate the size information.
    size_t total_size{sizeof(size_t)};

    if constexpr (use_memcpy_fast_path)
    {
      // Built-in arithmetic types don't require iteration.
      // Note: Enums are excluded as they may have custom Codecs (e.g., DirectFormatCodec).
      // std::vector<bool> is also excluded because it stores bits via proxy references.
      total_size += sizeof(T) * arg.size();
    }
    else
    {
      // For other complex types it's essential to determine the exact size of each element.
      // For instance, in the case of a collection of std::string, we need to know the exact size
      // of each string as we will be copying them directly to our queue buffer.
      for (auto const& elem : arg)
      {
        total_size += Codec<T>::compute_encoded_size(conditional_arg_size_cache, elem);
      }
    }

    return total_size;
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, Arg&& arg)
  {
    Codec<size_t>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, arg.size());

    if constexpr (use_memcpy_fast_path)
    {
      // Built-in arithmetic types don't require iteration.
      // Note: Enums are excluded as they may have custom Codecs (e.g., DirectFormatCodec).
      // std::vector<bool> is also excluded because it is not backed by a contiguous bool array.
      if (!arg.empty())
      {
        std::memcpy(buffer, arg.data(), sizeof(T) * arg.size());
      }
      buffer += sizeof(T) * arg.size();
    }
    else
    {
      if constexpr (std::is_rvalue_reference_v<Arg&&>)
      {
        for (auto&& elem : arg)
        {
          Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index,
                           std::move(elem));
        }
      }
      else
      {
        for (auto const& elem : arg)
        {
          Codec<T>::encode(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, elem);
        }
      }
    }
  }

  static auto decode_arg(std::byte*& buffer)
  {
#if defined(_WIN32)
    if constexpr (std::disjunction_v<std::is_same<T, wchar_t*>, std::is_same<T, wchar_t const*>,
                                     std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>>)
    {
      // Read the size of the vector
      size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);

      std::vector<std::string> encoded_values;
      encoded_values.reserve(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        std::wstring_view v = Codec<T>::decode_arg(buffer);
        encoded_values.emplace_back(detail::utf8_encode(v));
      }

      return encoded_values;
    }
    else
    {
#endif
      using ReturnType = decltype(Codec<T>::decode_arg(buffer));
      using ReboundAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<ReturnType>;
      std::vector<ReturnType, ReboundAllocator> arg;

      // Read the size of the vector
      size_t const number_of_elements = Codec<size_t>::decode_arg(buffer);
      arg.reserve(number_of_elements);

      for (size_t i = 0; i < number_of_elements; ++i)
      {
        auto elem = Codec<T>::decode_arg(buffer);
        if constexpr (std::is_move_constructible_v<ReturnType>)
        {
          arg.emplace_back(std::move(elem));
        }
        else
        {
          arg.emplace_back(elem);
        }
      }

      return arg;
#if defined(_WIN32)
    }
#endif
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE

/**
 * std::vector<bool> is a special container that uses proxy references rather than
 * actual bool&. This prevents the generic ranges formatter from working on libc++,
 * so we provide an explicit specialization.
 */
template <>
struct fmtquill::formatter<std::vector<bool>>
{
  constexpr auto parse(fmtquill::format_parse_context& ctx) -> decltype(ctx.begin())
  {
    return ctx.begin();
  }

  auto format(std::vector<bool> const& vec, fmtquill::format_context& ctx) const -> fmtquill::format_context::iterator
  {
    auto out = ctx.out();
    *out++ = '[';
    for (size_t i = 0; i < vec.size(); ++i)
    {
      if (i > 0)
      {
        *out++ = ',';
        *out++ = ' ';
      }
      bool const val = vec[i];
      out = fmtquill::format_to(out, "{}", val);
    }
    *out++ = ']';
    return out;
  }
};
