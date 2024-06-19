/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"

namespace quill::utility
{
/**
 * StringRef is used to specify that a string argument should be passed by reference instead of by
 * value, ensuring that no copy of the string is made.
 *
 * Note that by default, all strings, including C strings and std::string_view, are copied before
 * being passed to the backend. To pass strings by reference, they must be wrapped in a StringRef.
 *
 * Use this with caution, as the backend will parse the string asynchronously. The wrapped string
 * must have a valid lifetime and should not be modified.
 */
class StringRef
{
public:
  explicit StringRef(std::string const& str) : _str_view(str){};
  explicit StringRef(std::string_view str) : _str_view(str){};
  explicit StringRef(char const* str) : _str_view(str, strlen(str)){};
  StringRef(char const* str, size_t size) : _str_view(str, size){};

  QUILL_NODISCARD std::string_view const& get_string_view() const noexcept { return _str_view; }

private:
  std::string_view _str_view;
};
} // namespace quill::utility

/***/
template <>
struct quill::detail::ArgSizeCalculator<quill::utility::StringRef>
{
  static size_t calculate(std::vector<size_t>& conditional_arg_size_cache,
                          quill::utility::StringRef const& no_copy) noexcept
  {
    return sizeof(size_t) + sizeof(uintptr_t);
  }
};

/***/
template <>
struct quill::detail::Encoder<quill::utility::StringRef>
{
  static void encode(std::byte*& buffer, std::vector<size_t> const& conditional_arg_size_cache,
                     uint32_t& conditional_arg_size_cache_index, quill::utility::StringRef const& no_copy) noexcept
  {
    char const* data = no_copy.get_string_view().data();
    std::memcpy(buffer, &data, sizeof(uintptr_t));
    buffer += sizeof(uintptr_t);

    size_t const size = no_copy.get_string_view().size();
    std::memcpy(buffer, &size, sizeof(size_t));
    buffer += sizeof(size_t);
  }
};

/***/
template <>
struct quill::detail::Decoder<quill::utility::StringRef>
{
  static void decode(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    char const* data;
    std::memcpy(&data, buffer, sizeof(uintptr_t));
    buffer += sizeof(uintptr_t);

    size_t size;
    std::memcpy(&size, buffer, sizeof(size_t));
    buffer += sizeof(size_t);

    args_store->push_back(std::string_view{data, size});
  }
};