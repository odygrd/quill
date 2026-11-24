/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#ifndef QUILL_MODULE
  #include <memory>
  #include <string>
  #include <string_view>
  #include <type_traits>
  #include <vector>
#endif

#include "quill/bundled/fmt/base.h"
#include "quill/core/Attributes.h"

QUILL_BEGIN_NAMESPACE

namespace detail
{
class DynamicArgList
{
  struct Node
  {
    virtual ~Node() = default;
    std::unique_ptr<Node> next;
  };

  template <typename T>
  struct TypedNode : Node
  {
    template <typename Arg>
    explicit TypedNode(Arg&& arg) : value(_construct(static_cast<Arg&&>(arg)))
    {
    }

  private:
    template <typename Arg>
    static T _construct(Arg&& arg)
    {
      if constexpr (std::is_move_constructible<T>::value)
      {
        return T(static_cast<Arg&&>(arg));
      }
      else
      {
        // For copy-only types: arg is a named rvalue reference, which is an lvalue expression.
        // This lvalue can bind to const T& (the copy constructor parameter), resulting in a copy.
        return T(arg);
      }
    }

  public:
    T value;
  };

  std::unique_ptr<Node> _head;

public:
  template <typename T, typename Arg>
  T& push(Arg&& arg)
  {
    auto new_node = std::unique_ptr<TypedNode<T>>(new TypedNode<T>(static_cast<Arg&&>(arg)));
    T& value = new_node->value;
    new_node->next = static_cast<std::unique_ptr<Node>&&>(_head);
    _head = static_cast<std::unique_ptr<TypedNode<T>>&&>(new_node);
    return value;
  }
};
} // namespace detail

QUILL_BEGIN_EXPORT

/**
 * Similar to fmt::dynamic_arg_store but better suited to our needs
 * e.g. does not include ``<functional>`` and requires less space
 */
class DynamicFormatArgStore
{
private:
  // Storage of basic_format_arg must be contiguous.
  std::vector<fmtquill::basic_format_arg<fmtquill::format_context>> _data;

  // Storage of arguments not fitting into basic_format_arg must grow
  // without relocation because items in data_ refer to it.
  detail::DynamicArgList _dynamic_arg_list;

  /** An object of defer_destruction() */
  struct QueuedObject
  {
    void* object;
    void (*destroy)(void*);
  };

  std::vector<QueuedObject> _queued_objects;
  bool _has_string_related_type{false};
  bool _has_custom_type{false};

  template <typename T>
  void emplace_arg(T&& arg)
  {
    _data.emplace_back(static_cast<T&&>(arg));
  }

public:
  DynamicFormatArgStore() = default;

  QUILL_NODISCARD int size() const { return static_cast<int>(_data.size()); }

  QUILL_NODISCARD fmtquill::basic_format_arg<fmtquill::format_context> const* data() const
  {
    return _data.data();
  }

  /**
    Adds an argument for later passing to a formatting function.

    **Example**::

      quill::DynamicFormatArgStore store;
      store.push_back(42);
      store.push_back("abc");
      store.push_back(1.5f);
      std::string result = fmtquill::vformat(
        "{} and {} and {}", fmtquill::basic_format_args<fmtquill::format_context>{store.data(), store.size()});
  */
  template <typename T>
  void push_back(T&& arg)
  {
    _push_back<false>(static_cast<T&&>(arg));
  }

  /**
   * Leaves the destruction of an object to clear(). The decoder of an argument that lives in the
   * frontend queue calls this instead of destroying the object, because the backend decodes a
   * record again when the decoder of a later argument throws.
   */
  template <typename T>
  void defer_destruction(T* object)
  {
    _queued_objects.push_back(
      QueuedObject{object, [](void* queued_object) { static_cast<T*>(queued_object)->~T(); }});
  }

  /**
   * Adds an argument that lives in the frontend queue, where it stays alive until clear(). The
   * store does not copy or move the object: it refers to it, or keeps what the object is
   * formatted through, its conversion to std::string or the value that it maps to.
   */
  template <typename T>
  void push_back_queued(T& object)
  {
    _push_back<true>(static_cast<T&&>(object));
  }

  /**
   * Erase all elements from the store
   * @param destroy_queued_objects true to destroy the objects of defer_destruction(), when the
   * record they belong to is consumed
   */
  void clear(bool destroy_queued_objects = false)
  {
    _data.clear();
    _dynamic_arg_list = detail::DynamicArgList{};

    if (QUILL_UNLIKELY(!_queued_objects.empty()))
    {
      _clear_queued_objects(destroy_queued_objects);
    }

    _has_string_related_type = false;
    _has_custom_type = false;
  }

  QUILL_NODISCARD bool has_string_related_type() const noexcept { return _has_string_related_type; }

  /**
   * @return true if an argument is formatted by a fmtquill::formatter specialization. Unlike the
   * formatting of a built-in type, it can depend on the other arguments, which the formatter
   * reads through its format context.
   */
  QUILL_NODISCARD bool has_custom_type() const noexcept { return _has_custom_type; }

private:
  /** true if an argument of type T is formatted by its own fmtquill::formatter specialization */
  template <typename T>
  static constexpr bool _has_user_formatter =
    (fmtquill::detail::mapped_type_constant<T, typename fmtquill::format_context::char_type>::value ==
     fmtquill::detail::type::custom_type) &&
    fmtquill::is_formattable<T, typename fmtquill::format_context::char_type>::value;

  /** @tparam TIsQueued true for an argument of push_back_queued() */
  template <bool TIsQueued, typename T>
  void _push_back(T&& arg)
  {
    using char_type = typename fmtquill::format_context::char_type;
    using bare_type = std::remove_cv_t<std::remove_reference_t<T>>;
    constexpr auto mapped_type = fmtquill::detail::mapped_type_constant<bare_type, char_type>::value;
    // A type with its own formatter is stored as itself even when it also converts to std::string,
    // otherwise the conversion would replace the user formatter (or throw, e.g. nlohmann::json).
    // A type without a formatter is still formatted through its conversion to std::string
    constexpr bool store_as_std_string =
      !_has_user_formatter<bare_type> && std::is_convertible_v<bare_type, std::string>;
    using stored_type = std::conditional_t<store_as_std_string, std::string, bare_type>;

    if constexpr (!(std::is_same_v<bare_type, std::string_view> || std::is_same_v<bare_type, fmtquill::string_view> ||
                    (mapped_type != fmtquill::detail::type::cstring_type &&
                     mapped_type != fmtquill::detail::type::string_type &&
                     mapped_type != fmtquill::detail::type::custom_type)))
    {
      if constexpr (TIsQueued && !store_as_std_string)
      {
        // The argument stays alive until clear(), so the store refers to it instead of copying it
        emplace_arg(arg);
      }
      else
      {
        emplace_arg(_dynamic_arg_list.push<stored_type>(static_cast<T&&>(arg)));
      }
    }
    else
    {
      emplace_arg(static_cast<T&&>(arg));
    }

    if constexpr (std::is_same_v<bare_type, std::string_view> || std::is_same_v<bare_type, fmtquill::string_view> ||
                  (mapped_type == fmtquill::detail::type::cstring_type) ||
                  (mapped_type == fmtquill::detail::type::string_type) ||
                  (mapped_type == fmtquill::detail::type::custom_type) ||
                  (mapped_type == fmtquill::detail::type::char_type))
    {
      _has_string_related_type = true;
    }

    if constexpr (_has_user_formatter<bare_type>)
    {
      _has_custom_type = true;
    }
  }

  /** Kept out of clear(), which runs for every record */
  QUILL_ATTRIBUTE_COLD void _clear_queued_objects(bool destroy_queued_objects) noexcept
  {
    if (destroy_queued_objects)
    {
      for (QueuedObject const& queued_object : _queued_objects)
      {
        queued_object.destroy(queued_object.object);
      }
    }

    _queued_objects.clear();
  }
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
