#pragma once

#include "Os.h"
#include "Utilities.h"

namespace quill
{
namespace detail
{
/**
 * An allocator class to allocate aligned memory
 * @tparam T
 */
template <typename T>
class CacheAlignedAllocator
{
public:
  using value_type = T;

  CacheAlignedAllocator() noexcept {}

  template <typename U>
  CacheAlignedAllocator(CacheAlignedAllocator<U> const&) noexcept
  {
  }

  /**
   * allocate
   * @param n
   * @return
   */
  value_type* allocate(std::size_t n)
  {
    return static_cast<value_type*>(detail::aligned_alloc(CACHELINE_SIZE, n * sizeof(value_type)));
  }

  /**
   * free
   * @param p
   */
  void deallocate(value_type* p, std::size_t) noexcept { detail::aligned_free(p); }
};

template <typename T, typename U>
bool operator==(CacheAlignedAllocator<T> const&, CacheAlignedAllocator<U> const&) noexcept
{
  return true;
}

template <typename T, typename U>
bool operator!=(CacheAlignedAllocator<T> const& x, CacheAlignedAllocator<U> const& y) noexcept
{
  return !(x == y);
}

} // namespace detail
} // namespace quill