/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once
#include "quill/detail/misc/StringView.h"
#include "quill/detail/misc/TypeTraits.h"
#include <cstdint>
#include <string>

namespace quill
{
namespace detail
{

/** Serialize Traits **/

/**
 * We use a second typename for enable_if specialization
 * @tparam T
 */
template <typename T, typename = void>
struct type_descriptor_helper
{
  // there is no value for non-supported types - compilation will fail
};

template <>
struct type_descriptor_helper<bool>
{
  static constexpr char const* value = "%B";
};

template <>
struct type_descriptor_helper<short>
{
  static constexpr char const* value = "%IS";
};

template <>
struct type_descriptor_helper<int>
{
  static constexpr char const* value = "%I";
};

template <>
struct type_descriptor_helper<long>
{
  static constexpr char const* value = "%IL";
};

template <>
struct type_descriptor_helper<long long>
{
  static constexpr char const* value = "%ILL";
};

template <>
struct type_descriptor_helper<unsigned short>
{
  static constexpr char const* value = "%UIS";
};

template <>
struct type_descriptor_helper<unsigned int>
{
  static constexpr char const* value = "%UI";
};

template <>
struct type_descriptor_helper<unsigned long>
{
  static constexpr char const* value = "%UIL";
};

template <>
struct type_descriptor_helper<unsigned long long>
{
  static constexpr char const* value = "%UILL";
};

template <>
struct type_descriptor_helper<double>
{
  static constexpr char const* value = "%D";
};

template <>
struct type_descriptor_helper<long double>
{
  static constexpr char const* value = "%LD";
};

template <>
struct type_descriptor_helper<float>
{
  static constexpr char const* value = "%F";
};

template <>
struct type_descriptor_helper<char>
{
  static constexpr char const* value = "%C";
};

template <>
struct type_descriptor_helper<unsigned char>
{
  static constexpr char const* value = "%UC";
};

template <>
struct type_descriptor_helper<signed char>
{
  static constexpr char const* value = "%CS";
};

template <>
struct type_descriptor_helper<void*>
{
  static constexpr char const* value = "%P";
};

template <>
struct type_descriptor_helper<char const*>
{
  static constexpr char const* value = "%SC";
};

template <>
struct type_descriptor_helper<char*>
{
  static constexpr char const* value = "%S";
};

template <>
struct type_descriptor_helper<std::string>
{
  static constexpr char const* value = "%S";
};

#if defined(QUILL_USE_STRING_VIEW)
template <>
struct type_descriptor_helper<std_string_view<char>>
{
  static constexpr char const* value = "%S";
};
#endif

/**
 * Explicit enum specialization
 * @tparam T
 */
template <typename T>
struct type_descriptor_helper<T, std::enable_if_t<std::is_enum<T>::value>>
{
  // for enums we want to copy the underlying type
  using enum_underlying_t = std::underlying_type_t<T>;
  static constexpr char const* value = type_descriptor_helper<enum_underlying_t>::value;
};

template <typename T>
struct type_descriptor : public type_descriptor_helper<remove_cvref_t<std::decay_t<T>>>
{
};

template <typename... Args>
inline typename std::enable_if<sizeof...(Args) == 0>::type construct_type_descriptor_string(std::string&)
{
}

template <typename Arg, typename... Args>
inline void construct_type_descriptor_string(std::string& s)
{
  s += type_descriptor<Arg>::value;
  construct_type_descriptor_string<Args...>(s);
}

template <typename... Args>
inline std::string type_descriptor_string()
{
  std::string s;
  s.reserve(8);
  construct_type_descriptor_string<Args...>(s);
  return s;
}

} // namespace detail
} // namespace quill