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

#include <cstring>
#include <type_traits>

QUILL_BEGIN_NAMESPACE

/**
 * @brief Provides serialization (codec) functionality for complex user-defined types.
 *
 * This codec minimizes overhead on the hot-path by directly using memcpy or
 * placement new to serialize objects into the SPSC buffer,
 *
 * This approach avoids expensive string formatting on the hot path.
 *
 * For a trivially copyable types it requires a default constructor
 * For a non trivially copyable types it requires valid copy constructor and move constructor.
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

template <typename T>
struct DeferredFormatCodec
{
  // Checks like this Will fail for types with private ctors and friends declarations
  //  static_assert(std::is_copy_constructible_v<T>, "T must be copy constructible");
  static constexpr bool is_trivially_copyable = std::is_trivially_copyable_v<T>;

  static size_t compute_encoded_size(detail::SizeCacheVector&, T const&) noexcept
  {
    if constexpr (is_trivially_copyable)
    {
      return sizeof(T);
    }
    else
    {
      return sizeof(T) + alignof(T);
    }
  }

  static void encode(std::byte*& buffer, detail::SizeCacheVector const&, uint32_t&, T const& arg) noexcept
  {
    if constexpr (is_trivially_copyable)
    {
      std::memcpy(buffer, &arg, sizeof(arg));
      buffer += sizeof(arg);
    }
    else
    {
      auto aligned_ptr = align_pointer(buffer, alignof(T));
      new (static_cast<void*>(aligned_ptr)) T(arg);
      buffer += sizeof(T) + alignof(T);
    }
  }

  static T decode_arg(std::byte*& buffer)
  {
    if constexpr (is_trivially_copyable)
    {
      T arg;

      // Cast to void* to silence compiler warning about private members
      std::memcpy(static_cast<void*>(&arg), buffer, sizeof(arg));

      buffer += sizeof(arg);
      return arg;
    }
    else
    {
      auto aligned_ptr = align_pointer(buffer, alignof(T));
      auto* tmp = reinterpret_cast<T*>(aligned_ptr);

      // Take a copy
      T arg{*tmp};

      if constexpr (!std::is_trivially_destructible_v<T>)
      {
        // Destroy tmp
        tmp->~T();
      }

      buffer += sizeof(T) + alignof(T);
      return arg;
    }
  }

  static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
  {
    args_store->push_back(decode_arg(buffer));
  }

private:
  static std::byte* align_pointer(void* pointer, size_t alignment) noexcept
  {
    return reinterpret_cast<std::byte*>((reinterpret_cast<uintptr_t>(pointer) + (alignment - 1ul)) &
                                        ~(alignment - 1ul));
  }
};

QUILL_END_NAMESPACE