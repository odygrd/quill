#pragma once

#include <tuple>
#include <utility>
#include <type_traits>

namespace quill::detail
{

  // TODO:: Add invoke impl

  /**
  * Apply Implementation for C+14 support
   * @tparam _Fn
   * @tparam _Tuple
   * @tparam _Idx
   * @param __f
   * @param __t
   * @return
   */
  template <typename _Fn, typename _Tuple, size_t... _Idx>
  constexpr decltype(auto) apply_impl(_Fn&& __f, _Tuple&& __t, std::integer_sequence<size_t, _Idx...>)
  {
    return invoke(std::forward<_Fn>(__f), std::get<_Idx>(std::forward<_Tuple>(__t))...);
  }

  /**
   * Apply
   * @tparam _Fn
   * @tparam _Tuple
   * @param __f
   * @param __t
   * @return
   */
  template <typename _Fn, typename _Tuple>
  constexpr decltype(auto) apply(_Fn&& __f, _Tuple&& __t)
  {
    using _Indices = std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<_Tuple>>>;
    return apply_impl(std::forward<_Fn>(__f), std::forward<_Tuple>(__t), _Indices{});
  }
}