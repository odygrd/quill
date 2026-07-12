/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"
#include "quill/core/InlinedVector.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

QUILL_BEGIN_NAMESPACE

#if defined(_WIN32) && defined(_MSC_VER)
  // silence msvc warning C4702: unreachable code
  #pragma warning(push)
  #pragma warning(disable : 4702)
#endif

/**
 * @brief Provides serialization (codec) functionality for complex user-defined types.
 *
 * This codec minimizes overhead on the hot-path by directly using std::memcpy or
 * placement new to serialize objects into the SPSC buffer,
 *
 * This approach avoids expensive string formatting on the hot path.
 *
 * Non-trivially-copyable types require a publicly accessible copy or move constructor. A
 * `friend struct quill::DeferredFormatCodec<T>` declaration can make a private default
 * constructor available for the trivially-copyable memcpy recipe, but it does not make private
 * copy or move constructors usable by the backend's format-argument store.
 *
 * @warning For non-trivially-copyable types, copy/move constructors used during encoding or
 *          decoding, and the destructor, must not throw. A failed record remains uncommitted, but
 *          Quill cannot destroy an earlier deferred argument from that record.
 *
 * Thread-Safety for non trivially copyable types:
 * It is the user's responsibility to ensure that an non trivially copyable type remains
 * thread-safe after being copied. For example, if the object contains a `shared_ptr`, ensure that
 * its underlying value will not be modified.
 *
 * Example usage:
 *
 * \code{.cpp}
 * class User
 * {
 * public:
 *   User(std::string name, std::string surname, uint32_t age)
 *     : name(std::move(name)), surname(std::move(surname)), age(age)
 *   {
 *     favorite_colors.push_back("red");
 *     favorite_colors.push_back("blue");
 *     favorite_colors.push_back("green");
 *   };
 *
 *   std::string name;
 *   std::string surname;
 *   uint32_t age;
 *   std::vector<std::string> favorite_colors;
 * };
 *
 * template <>
 * struct fmtquill::formatter<User>
 * {
 *   constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
 *
 *   auto format(::User const& user, format_context& ctx) const
 *   {
 *     return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
 *                                user.name, user.surname, user.age, user.favorite_colors);
 *   }
 * };
 *
 * template <>
 * struct quill::Codec<User> : quill::DeferredFormatCodec<User>
 * {
 * };
 *
 * int main()
 * {
 *   // ... init code
 *   User user_1{"Super", "User", 1};
 *   LOG_INFO(logger, "User is [{}]", user_1);
 * }
 * \endcode
 */

QUILL_BEGIN_EXPORT

template <typename T>
struct DeferredFormatCodec
{
private:
  // This check takes a `friend struct quill::DeferredFormatCodec<T>` declaration into account,
  // unlike std::is_default_constructible. It supports the documented trivially-copyable recipe
  // whose default constructor is private.
  template <typename U>
  static auto is_default_constructible(int) -> decltype(U(), std::true_type{});

  template <typename>
  static auto is_default_constructible(...) -> std::false_type;

public:
  static constexpr bool use_memcpy =
    std::conjunction_v<std::is_trivially_copyable<T>, decltype(is_default_constructible<T>(0))>;

  static size_t compute_encoded_size(detail::SizeCacheVector&, T const&) noexcept
  {
    if constexpr (use_memcpy)
    {
      return sizeof(T);
    }
    else
    {
      // If it’s misaligned, the worst-case scenario is when the pointer is off by one byte from an alignment boundary
      return sizeof(T) + alignof(T) - 1;
    }
  }

  template <typename Arg>
  static void encode(std::byte*& buffer, detail::SizeCacheVector const&, uint32_t&, Arg&& arg)
  {
    if constexpr (use_memcpy)
    {
      std::memcpy(buffer, &arg, sizeof(arg));
      buffer += sizeof(arg);
    }
    else
    {
      auto aligned_ptr = align_pointer(buffer, alignof(T));

      if constexpr (std::is_rvalue_reference_v<Arg&&> && std::is_move_constructible_v<T>)
      {
        new (static_cast<void*>(aligned_ptr)) T(std::move(arg));
      }
      else
      {
        static_assert(std::is_copy_constructible_v<T>, "T is not publicly copy-constructible!");
        new (static_cast<void*>(aligned_ptr)) T(arg);
      }

      buffer += sizeof(T) + alignof(T) - 1;
    }
  }

  static T decode_arg(std::byte*& buffer)
  {
    if constexpr (use_memcpy)
    {
      T arg;

      // Cast to void* to silence compiler warning about private members
      std::memcpy(static_cast<void*>(&arg), buffer, sizeof(arg));

      buffer += sizeof(arg);
      return arg;
    }
    else
    {
      static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>,
                    "T must be publicly move or copy constructible");

      auto* aligned_ptr = align_pointer(buffer, alignof(T));
      auto* tmp = std::launder(reinterpret_cast<T*>(aligned_ptr));
      buffer += sizeof(T) + alignof(T) - 1;

      // Guarantee *tmp is destroyed even if the move/copy below throws. The original object
      // was placement-new'd into the queue buffer during encode().
      struct DestroyGuard
      {
        T* ptr;
        ~DestroyGuard()
        {
          if constexpr (!std::is_trivially_destructible_v<T>)
          {
            ptr->~T();
          }
        }
      } destroy_guard{tmp};

      if constexpr (std::is_move_constructible_v<T>)
      {
        // Move into the backend value; DestroyGuard still owns the queue-buffer object.
        return T{std::move(*tmp)};
      }
      else
      {
        // Keep the return as an explicit copy. `return T{*tmp};` is preferred over `return *tmp;`
        // since the latter can still prefer a deleted move constructor for copy-only types.
        return T{*tmp};
      }
    }
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    QUILL_TRY { args_store->push_back(decode_arg(buffer)); }
#if !defined(QUILL_NO_EXCEPTIONS)
    QUILL_CATCH_ALL()
    {
      // Fall back to a placeholder so the surrounding log line is still produced. Without this,
      // a throwing decode would propagate up and discard the whole transit event.
      static constexpr char fallback[] = "[Quill deferred decode failed]";
      args_store->push_back(fmtquill::string_view{fallback, sizeof(fallback) - 1u});
    }
#endif
  }

private:
  static std::byte* align_pointer(void* pointer, size_t alignment) noexcept
  {
    return reinterpret_cast<std::byte*>((reinterpret_cast<uintptr_t>(pointer) + (alignment - 1ul)) &
                                        ~(alignment - 1ul));
  }
};

QUILL_END_EXPORT

#if defined(_WIN32) && defined(_MSC_VER)
  #pragma warning(pop)
#endif

QUILL_END_NAMESPACE
