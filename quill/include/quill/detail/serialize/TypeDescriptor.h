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

/**
 * Type descriptor enum values
 */
enum TypeDescriptor : char
{
  Bool = 'a',
  Short = 'b',
  Int = 'c',
  Long = 'd',
  LongLong = 'e',
  UnsignedShort = 'f',
  UnsignedInt = 'g',
  UnsignedLong = 'h',
  UnsignedLongLong = 'i',
  Double = 'j',
  LongDouble = 'k',
  Float = 'l',
  Char = 'm',
  UnsignedChar = 'n',
  SignedChar = 'o',
  VoidPtr = 'p',
  String = 'q'
};

/** Serialize Traits **/

/**
 * We use a second typename for enable_if specialization
 */
template <typename T, typename = void>
struct type_descriptor_helper
{
  // there is no value for non-supported types - compilation will fail
};

template <>
struct type_descriptor_helper<bool>
{
  static constexpr char const value = TypeDescriptor::Bool;
};

template <>
struct type_descriptor_helper<short>
{
  static constexpr char const value = TypeDescriptor::Short;
};

template <>
struct type_descriptor_helper<int>
{
  static constexpr char const value = TypeDescriptor::Int;
};

template <>
struct type_descriptor_helper<long>
{
  static constexpr char const value = TypeDescriptor::Long;
};

template <>
struct type_descriptor_helper<long long>
{
  static constexpr char const value = TypeDescriptor::LongLong;
};

template <>
struct type_descriptor_helper<unsigned short>
{
  static constexpr char const value = TypeDescriptor::UnsignedShort;
};

template <>
struct type_descriptor_helper<unsigned int>
{
  static constexpr char const value = TypeDescriptor::UnsignedInt;
};

template <>
struct type_descriptor_helper<unsigned long>
{
  static constexpr char const value = TypeDescriptor::UnsignedLong;
};

template <>
struct type_descriptor_helper<unsigned long long>
{
  static constexpr char const value = TypeDescriptor::UnsignedLongLong;
};

template <>
struct type_descriptor_helper<double>
{
  static constexpr char const value = TypeDescriptor::Double;
};

template <>
struct type_descriptor_helper<long double>
{
  static constexpr char const value = TypeDescriptor::LongDouble;
};

template <>
struct type_descriptor_helper<float>
{
  static constexpr char const value = TypeDescriptor::Float;
};

template <>
struct type_descriptor_helper<char>
{
  static constexpr char const value = TypeDescriptor::Char;
};

template <>
struct type_descriptor_helper<unsigned char>
{
  static constexpr char const value = TypeDescriptor::UnsignedChar;
};

template <>
struct type_descriptor_helper<signed char>
{
  static constexpr char const value = TypeDescriptor::SignedChar;
};

template <>
struct type_descriptor_helper<void*>
{
  static constexpr char const value = TypeDescriptor::VoidPtr;
};

template <>
struct type_descriptor_helper<char const*>
{
  static constexpr char const value = TypeDescriptor::String;
};

template <>
struct type_descriptor_helper<char*>
{
  static constexpr char const value = TypeDescriptor::String;
};

template <>
struct type_descriptor_helper<std::string>
{
  static constexpr char const value = TypeDescriptor::String;
};

#if defined(QUILL_USE_STRING_VIEW)
template <>
struct type_descriptor_helper<std_string_view<char>>
{
  static constexpr char const value = TypeDescriptor::String;
};
#endif

template <typename T>
struct type_descriptor : public type_descriptor_helper<remove_cvref_t<std::decay_t<T>>>
{
};

template <typename... Args>
inline std::enable_if_t<sizeof...(Args) == 0> construct_type_descriptor_string(std::string&)
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