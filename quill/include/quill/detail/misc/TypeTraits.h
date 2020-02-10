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
 * @tparam T
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
 * @tparam T
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
} // namespace detail
} // namespace quill