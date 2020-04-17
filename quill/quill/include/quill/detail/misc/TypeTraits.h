/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <functional>
#include <string>
#include <tuple>
#include <type_traits>

namespace quill
{
namespace detail
{
/**
 * We use promoted to convert all char const* or char* to a std::string when the Logger is creating a LogRecord
 * This is because the record is processed later by the backend thread and the char* could be out of scope
 */
template <typename T>
struct Promoted
{
  using type = T;
};

/**
 * Specialization for char const*
 * @see promoted
 */
template <>
struct Promoted<char const*>
{
  using type = std::string;
};

/**
 * Specialization for wchar_t const*
 * @see promoted
 */
template <>
struct Promoted<wchar_t const*>
{
  using type = std::wstring;
};

/**
 * Specialization for char*
 * @see promoted
 */
template <>
struct Promoted<char*>
{
  using type = std::string;
};

/**
 * Specialization for char*
 * @see promoted
 */
template <>
struct Promoted<wchar_t*>
{
  using type = std::wstring;
};

/**
 * Helper to unwrap reference wrapper
 */
template <class T>
struct UnwrapRefWrapper
{
  using type = T;
};

template <class T>
struct UnwrapRefWrapper<std::reference_wrapper<T>>
{
  using type = T&;
};

/**
 * @see promoted
 */
template <typename T>
using PromotedTypeT = typename UnwrapRefWrapper<typename Promoted<std::decay_t<T>>::type>::type;

/**
 * any_is_same
 * Detect if the parameter pack contains this type in it's arguments.
 * This is used to find if any of the passed arguments were wide characters or strings and convert
 * them
 */
template <typename TSame, typename TFirst, typename... TRest>
struct any_is_same
{
  static constexpr bool value = std::is_same<TSame, std::decay_t<TFirst>>::value ||
    any_is_same<TSame, std::decay_t<TRest>...>::value;
};

template <typename TSame, typename TFirst>
struct any_is_same<TSame, TFirst> : std::is_same<TSame, std::decay_t<TFirst>>
{
};

} // namespace detail
} // namespace quill