/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"

namespace quill
{
namespace detail
{

/**
 * Include string_view if it is supported
 */
#if (QUILL_HAS_INCLUDE(<string_view>) && (__cplusplus > 201402L || defined(_LIBCPP_VERSION))) ||   \
  (defined(_MSVC_LANG) && (_MSVC_LANG > 201402L) && (_MSC_VER >= 1910))

  #define QUILL_USE_STRING_VIEW
  #include <string_view>
template <typename Char>
using std_string_view = std::basic_string_view<Char>;
#endif

} // namespace detail
} // namespace quill