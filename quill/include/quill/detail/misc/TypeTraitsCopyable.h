/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * Below are type traits to determine whether an object is marked as copyable.
 * The traits are used to decide about
 *
 * a) it is safe to copy the object in the queue and safe to format it later
 * b) it could be unsafe to copy the object in the queue and we first have to format it on the caller thread
 *
 * Since with libfmt it is not possible to format pointers (except void*) we don't handle every
 * case with pointers.
 *
 * A safe to copy object meets one of the following criteria :
 * a) built arithmetic types
 * b) trivial types
 * c) strings
 * d) explicitly tagged by the user as copy_loggable with the QUILL_COPY_LOGGABLE macro
 * e) std::duration types
 * f) containers of the above types
 * g) std::pairs of the above types
 * h) std::tuples of the above types
 */

// clang-format off
namespace quill
{

/**
 * A struct to registered as copy_loggable via a specialization provided by the user.
 * This is here and not in the detail namespace so that the user can provide a specialization for user defined types
 */
template <typename T>
struct copy_loggable : std::false_type
{
};

namespace detail
{
/**
 * C++14 implementation of C++20's remove_cvref
 */
template< class T >
struct remove_cvref
{
  typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};

template< class T >
using remove_cvref_t = typename remove_cvref<T>::type;

/**
 * fmt::streamed detection
 */
#if FMT_VERSION >= 90000
template<typename T>
struct is_fmt_stream_view : std::false_type
{
};

template<typename T>
struct is_fmt_stream_view<fmt::detail::streamed_view<T>> : std::true_type
{
};

template<typename... Args>
constexpr bool has_fmt_stream_view_v = std::disjunction_v<is_fmt_stream_view<remove_cvref_t<Args>>...>;
#endif

/**************************************************************************************************/
/* Type Traits for copyable object detection */
/**************************************************************************************************/

/**
 * Used to detect if an object has a specific member.
 * This is different than enable_if_t which accepts a bool not a type
 */
template <typename T, typename R = void>
struct enable_if_type
{
  typedef R type;
};

template <typename T, typename R = void>
using enable_if_type_t = typename enable_if_type<T, R>::type;

/**
 * A trait to determine if the object is safe be copied as it is
 * Specialisations follow later
 * A copyable object is either a basic type, an stl container or a tagged one
 */
template <typename T, typename T2 = void>
struct is_copyable : std::false_type
{
};

template <typename T>
using is_copyable_t = typename is_copyable<remove_cvref_t<T>>::type;

template <typename T>
constexpr bool is_copyable_v = is_copyable<remove_cvref_t<T>>::value;

template<typename... Args>
using are_copyable_t = typename std::conjunction<is_copyable<remove_cvref_t<Args>>...>;

template<typename... Args>
constexpr bool are_copyable_v = std::conjunction_v<is_copyable<remove_cvref_t<Args>>...>;

/**
 * A trait to detect an object was tagged as copy_loggable
 */
template <typename T, typename T2 = void>
struct is_tagged_copyable : std::false_type
{
};

/**
 * Enable only when we have a tag
 */
template <typename T>
struct is_tagged_copyable<T, enable_if_type_t<typename T::copy_loggable>> : T::copy_loggable
{
};

template <typename T>
using is_tagged_copyable_t = typename is_tagged_copyable<remove_cvref_t<T>>::type;

template <typename T>
constexpr bool is_tagged_copyable_v = is_tagged_copyable<remove_cvref_t<T>>::value;

/**
 * Check if registered as copy_loggable via a specialization to copy_loggable struct provided by the user
 */
template <typename T>
using is_registered_copyable_t = typename copy_loggable<remove_cvref_t<T>>::type;

template <typename T>
constexpr bool is_registered_copyable_v = copy_loggable<remove_cvref_t<T>>::value;

/**
 * is std::string ?
 */
template <typename T>
struct is_string : std::false_type
{
};

template <typename CharT, typename Traits, typename Allocator>
struct is_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type
{
};

template <typename T>
using is_string_t = typename is_string<remove_cvref_t<T>>::type;

template <typename T>
constexpr bool is_string_v = is_string<remove_cvref_t<T>>::value;

/**
 * Check if each element of the pair is copyable
 */
template <typename T>
struct is_copyable_pair : std::false_type
{
};

/**
 * Enable only in the case of a std::pair and do the check
 */
template <typename T1, typename T2>
struct is_copyable_pair<std::pair<T1, T2>>
  : std::conjunction<is_copyable<remove_cvref_t<T1>>,
                is_copyable<remove_cvref_t<T2>>
                >
{
};

template <typename T>
using is_copyable_pair_t = typename is_copyable_pair<remove_cvref_t<T>>::type;

template <typename T>
constexpr bool is_copyable_pair_v = is_copyable_pair<remove_cvref_t<T>>::value;

/**
 * is it a container ?
 */
template <typename T, typename T2 = void>
struct is_container_helper : std::false_type
{
};

template <typename T>
struct is_container_helper<T, enable_if_type_t<typename T::iterator>> : std::true_type
{
};

/**
 * Check for a container but ignoring std::string
 */
template <typename T, typename T2 = void>
struct is_container : std::false_type
{
};

/**
 * Enable only when not a std::string as they also are like containers
 */
template <typename T>
struct is_container<T, std::enable_if_t<std::negation_v<is_string<T>>>>
  : is_container_helper<T>
{
};

template <typename T>
using is_container_t = typename is_container<remove_cvref_t<T>>::type;

template <typename T>
constexpr bool is_container_v = is_container<remove_cvref_t<T>>::value;

/**
 * Check if a container is copyable
 */
template <typename T, typename T2 = void>
struct is_copyable_container : std::false_type
{
};

/**
 * Enable the check only for containers. is_copyable is going to recursively call other if
 * for example we have std::vector<std::vector<int>>
 */
template <typename T>
struct is_copyable_container<T, std::enable_if_t<is_container_v<T>>>
  : is_copyable<remove_cvref_t<typename T::value_type>>
{
};

template <typename T>
using is_copyable_container_t = typename is_copyable_container<remove_cvref_t<T>>::type;

template <typename T>
constexpr bool is_copyable_container_v = is_copyable_container<remove_cvref_t<T>>::value;

/**
 * check for copyable elements in tuples
 */
template <typename T>
struct is_copyable_tuple : std::false_type
{
};

template <typename... Ts>
struct is_copyable_tuple<std::tuple<Ts...>>
    : std::conjunction<is_copyable<remove_cvref_t<Ts>>...>
{
};

/**
 * A user defined object that was tagged by the user to be copied
 */
template <typename T>
struct is_user_defined_copyable : std::conjunction<std::is_class<T>,
                                              std::negation<std::is_trivial<T>>,
                                              is_tagged_copyable<T>
                                             >
{};

/**
 * A user defined object that was tagged by the user to be copied via an external template specialisation
 * to copy_logable
 */
template <typename T>
struct is_user_registered_copyable : std::conjunction<std::is_class<T>,
                                                 std::negation<std::is_trivial<T>>,
                                                 copy_loggable<T>
                                                 >
{};

/**
 * An object is copyable if it meets one of the following criteria
 */
template <typename T>
struct filter_copyable : std::disjunction<std::is_arithmetic<T>,
                                     is_string<T>,
                                     std::is_trivial<T>,
                                     is_user_defined_copyable<T>,
                                     is_user_registered_copyable<T>,
                                     is_copyable_pair<T>,
                                     is_copyable_tuple<T>,
                                     is_copyable_container<T>
                                     >
{};

/**
 * Specialisation of is_copyable we defined on top to apply our copyable filter
 */
template <typename T>
struct is_copyable<T, std::enable_if_t<filter_copyable<T>::value>> : std::true_type
{
};
// clang-format on

} // namespace detail
} // namespace quill