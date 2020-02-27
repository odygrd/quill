/*******************************************************************************
 * This file is part of the "https://github.com/blackmatov/invoke.hpp"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

/**
 * Invoke implementation for C++14
 */

#pragma once

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

#define INVOKE_HPP_NOEXCEPT_RETURN(...) \
    noexcept(noexcept(__VA_ARGS__)) { return __VA_ARGS__; }

#define INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(...) \
    noexcept(noexcept(__VA_ARGS__)) -> decltype (__VA_ARGS__) { return __VA_ARGS__; }

//
// void_t
//

namespace invoke_hpp
{
  namespace impl
  {
    template < typename... Args >
    struct make_void {
      using type = void;
    };
  }

  template < typename... Args >
  using void_t = typename impl::make_void<Args...>::type;
}

//
// is_reference_wrapper
//

namespace invoke_hpp
{
  namespace impl
  {
    template < typename T >
    struct is_reference_wrapper_impl
      : std::false_type {};

    template < typename U >
    struct is_reference_wrapper_impl<std::reference_wrapper<U>>
      : std::true_type {};
  }

  template < typename T >
  struct is_reference_wrapper
    : impl::is_reference_wrapper_impl<std::remove_cv_t<T>> {};
}

//
// invoke
//

namespace invoke_hpp
{
  namespace impl
  {
    //
    // invoke_member_object_impl
    //

    template
      <
        typename Base, typename F, typename Derived,
        typename std::enable_if_t<std::is_base_of<Base, std::decay_t<Derived>>::value, int> = 0
      >
    constexpr auto invoke_member_object_impl(F Base::* f, Derived&& ref)
    INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
      std::forward<Derived>(ref).*f)

    template
      <
        typename Base, typename F, typename RefWrap,
        typename std::enable_if_t<is_reference_wrapper<std::decay_t<RefWrap>>::value, int> = 0
      >
    constexpr auto invoke_member_object_impl(F Base::* f, RefWrap&& ref)
    INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
      ref.get().*f)

    template
      <
        typename Base, typename F, typename Pointer,
        typename std::enable_if_t<
          !std::is_base_of<Base, std::decay_t<Pointer>>::value &&
          !is_reference_wrapper<std::decay_t<Pointer>>::value
          , int> = 0
      >
    constexpr auto invoke_member_object_impl(F Base::* f, Pointer&& ptr)
    INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
      (*std::forward<Pointer>(ptr)).*f)

    //
    // invoke_member_function_impl
    //

    template
      <
        typename Base, typename F, typename Derived, typename... Args,
        typename std::enable_if_t<std::is_base_of<Base, std::decay_t<Derived>>::value, int> = 0
      >
    constexpr auto invoke_member_function_impl(F Base::* f, Derived&& ref, Args&&... args)
    INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
      (std::forward<Derived>(ref).*f)(std::forward<Args>(args)...))

    template
      <
        typename Base, typename F, typename RefWrap, typename... Args,
        typename std::enable_if_t<is_reference_wrapper<std::decay_t<RefWrap>>::value, int> = 0
      >
    constexpr auto invoke_member_function_impl(F Base::* f, RefWrap&& ref, Args&&... args)
    INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
      (ref.get().*f)(std::forward<Args>(args)...))

    template
      <
        typename Base, typename F, typename Pointer, typename... Args,
        typename std::enable_if_t<
          !std::is_base_of<Base, std::decay_t<Pointer>>::value &&
          !is_reference_wrapper<std::decay_t<Pointer>>::value
          , int> = 0
      >
    constexpr auto invoke_member_function_impl(F Base::* f, Pointer&& ptr, Args&&... args)
    INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
      ((*std::forward<Pointer>(ptr)).*f)(std::forward<Args>(args)...))
  }

  template
    <
      typename F, typename... Args,
      typename std::enable_if_t<!std::is_member_pointer<std::decay_t<F>>::value, int> = 0
    >
  constexpr auto invoke(F&& f, Args&&... args)
  INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
    std::forward<F>(f)(std::forward<Args>(args)...))

  template
    <
      typename F, typename T,
      typename std::enable_if_t<std::is_member_object_pointer<std::decay_t<F>>::value, int> = 0
    >
  constexpr auto invoke(F&& f, T&& t)
  INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
    impl::invoke_member_object_impl(std::forward<F>(f), std::forward<T>(t)))

  template
    <
      typename F, typename... Args,
      typename std::enable_if_t<std::is_member_function_pointer<std::decay_t<F>>::value, int> = 0
    >
  constexpr auto invoke(F&& f, Args&&... args)
  INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN(
    impl::invoke_member_function_impl(std::forward<F>(f), std::forward<Args>(args)...))
}

//
// invoke_result
//

namespace invoke_hpp
{
  namespace impl
  {
    struct invoke_result_impl_tag {};

    template < typename Void, typename F, typename... Args >
    struct invoke_result_impl {};

    template < typename F, typename... Args >
    struct invoke_result_impl<void_t<invoke_result_impl_tag, decltype(invoke_hpp::invoke(std::declval<F>(), std::declval<Args>()...))>, F, Args...> {
      using type = decltype(invoke_hpp::invoke(std::declval<F>(), std::declval<Args>()...));
    };
  }

  template < typename F, typename... Args >
  struct invoke_result
    : impl::invoke_result_impl<void, F, Args...> {};

  template < typename F, typename... Args >
  using invoke_result_t = typename invoke_result<F, Args...>::type;
}

//
// is_invocable
//

namespace invoke_hpp
{
  namespace impl
  {
    struct is_invocable_r_impl_tag {};

    template < typename Void, typename R, typename F, typename... Args >
    struct is_invocable_r_impl
      : std::false_type {};

    template < typename R, typename F, typename... Args >
    struct is_invocable_r_impl<void_t<is_invocable_r_impl_tag, invoke_result_t<F, Args...>>, R, F, Args...>
      : std::conditional_t<
        std::is_void<R>::value,
        std::true_type,
        std::is_convertible<invoke_result_t<F, Args...>, R>> {};
  }

  template < typename R, typename F, typename... Args >
  struct is_invocable_r
    : impl::is_invocable_r_impl<void, R, F, Args...> {};

  template < typename F, typename... Args >
  using is_invocable = is_invocable_r<void, F, Args...>;
}

//
// apply
//

namespace invoke_hpp
{
  namespace impl
  {
    template < typename F, typename Tuple, std::size_t... I >
    constexpr decltype(auto) apply_impl(F&& f, Tuple&& args, std::index_sequence<I...>)
    INVOKE_HPP_NOEXCEPT_RETURN(
      invoke_hpp::invoke(
        std::forward<F>(f),
        std::get<I>(std::forward<Tuple>(args))...))
  }

  template < typename F, typename Tuple >
  constexpr decltype(auto) apply(F&& f, Tuple&& args)
  INVOKE_HPP_NOEXCEPT_RETURN(
    impl::apply_impl(
      std::forward<F>(f),
      std::forward<Tuple>(args),
      std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>()))
}

#undef INVOKE_HPP_NOEXCEPT_RETURN
#undef INVOKE_HPP_NOEXCEPT_DECLTYPE_RETURN