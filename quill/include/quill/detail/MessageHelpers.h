#pragma once

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
struct promoted
{
  using type = T;
};

/**
 * Specialization for char const*
 * @see promoted
 */
template <>
struct promoted<char const*>
{
  using type = std::string;
};

/**
 * Specialization for char*
 * @see promoted
 */
template <>
struct promoted<char*>
{
  using type = std::string;
};

/**
 * Helper to unwrap reference wrapper
 * @tparam T
 */
template <class T>
struct unwrap_refwrapper
{
  using type = T;
};

template <class T>
struct unwrap_refwrapper<std::reference_wrapper<T>>
{
  using type = T&;
};

/**
 * @see promoted
 */
template <typename T>
using promoted_t = typename unwrap_refwrapper<typename promoted<std::decay_t<T>>::type>::type;

/**
 * Make tuple version to perfect forward to std::make_tuple promoting all char const* and char* to std::string
 */
template <typename... ArgsT>
[[using gnu: hot, always_inline]] inline auto make_tuple(ArgsT&&... args)
{
  return std::make_tuple<promoted_t<ArgsT>...>(std::forward<ArgsT>(args)...);
}

/**
 * This struct is used to give the the type of the tuple we would get from make_tuple
 * Resolve the type of the tuple we will get from make_tuple
 * @tparam ArgsT
 */
template <typename... ArgsT>
struct resolve_tuple
{
  using type = std::tuple<promoted_t<ArgsT>...>;
};
} // namespace quill::detail