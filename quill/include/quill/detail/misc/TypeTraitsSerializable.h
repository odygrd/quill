/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/TypeTraitsCopyable.h"

namespace quill
{
namespace detail
{
/**
 * Below are type traits to determine whether an object is serializable by our internal serialization.
 */

/**
 * Default is based on std::is_fundamental type as all of them are supported if it is true
 *
 * We serialize pointers as strings, the the pointer is not a string we still serialize and
 * print the pointer value. All this happens in the serialization logic, here we just pass it as true
 *
 * We also serialize enums to their underlying type
 *
 * Note:: fmt arg store does not look like it is supporting wstrings, so we don't support them
 */
template <typename T>
struct is_serializable_helper
  : public disjunction<std::is_fundamental<T>, std::is_same<std::string, T>>
{
};

template <>
struct is_serializable_helper<char*> : public std::true_type
{
};

template <>
struct is_serializable_helper<char const*> : public std::true_type
{
};

template <>
struct is_serializable_helper<void*> : public std::true_type
{
};

template <size_t N>
struct is_serializable_helper<char[N]> : public std::true_type
{
};

template <size_t N>
struct is_serializable_helper<char const[N]> : public std::true_type
{
};

/**
 * We won't be serializing arrays, expect char arrays
 */
template <class T, size_t N>
struct is_serializable_helper<T[N]> : public std::false_type
{
};

/**
 * The below type traits are not supported for serialization, mixing character types is disallowed
 * when using fmt store
 */
template <size_t N>
struct is_serializable_helper<wchar_t[N]> : public std::false_type
{
};

template <size_t N>
struct is_serializable_helper<wchar_t const[N]> : public std::false_type
{
};

template <>
struct is_serializable_helper<char16_t> : public std::false_type
{
};

template <>
struct is_serializable_helper<char32_t> : public std::false_type
{
};

template <typename T>
struct is_serializable : public is_serializable_helper<remove_cvref_t<std::decay_t<T>>>
{
};

/**
 * Check all are serializable
 */
template <typename... TArgs>
struct is_all_serializable : conjunction<is_serializable<TArgs>...>
{
};

} // namespace detail
} // namespace quill