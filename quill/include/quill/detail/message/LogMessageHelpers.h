#pragma once

#include <functional>
#include <string>
#include <tuple>
#include <type_traits>

namespace quill::detail
{
/**
 * We use promoted to convert all char const* to a std::string when we are creating a message from the
 * caller thread to the logger thread.
 * We want copies of strings rather than const char* pointers, since the object can go out of scope on the caller thread.
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
 * Specialization for char*
 * @see promoted
 */
template <>
struct Promoted<char*>
{
  using type = std::string;
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

} // namespace quill::detail