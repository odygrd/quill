.. title:: Recipes

.. _recipes:
.. _cheat_sheet:

Recipes
=======

Find an example for the type you want to log, a custom formatter or codec, or a compile-time setting.

.. contents:: On this page
   :local:
   :depth: 1
   :backlinks: none
   :class: quill-page-index

Introduction
------------
``LOG_*`` accepts `{fmt} format strings <https://fmt.dev/latest/syntax>`_ with ``{}`` placeholders. The
:doc:`overview` describes how arguments reach the backend. For example, a vector's elements are encoded
into contiguous queue storage without calling the vector's copy constructor.

Preprocessor Configuration Flags
--------------------------------
Set the flags you need in the build system.

.. code:: cmake

    add_compile_definitions(QUILL_NO_EXCEPTIONS)

Build Quill without exception support.

.. code:: cmake

    add_compile_definitions(QUILL_NO_THREAD_NAME_SUPPORT)

Disable thread-name retrieval on toolchains that lack it, such as older Windows Server (2012/2016) or Android.

.. code:: cmake

    add_compile_definitions(QUILL_USE_SEQUENTIAL_THREAD_ID)

Assign each thread a sequential ID on first use and retain it for the thread's lifetime. Reproducible IDs
require threads to initialize in the same order. When using the headers, place the definition below in
exactly one ``.cpp`` file. Any shared library that references the counter or cache must resolve both to
that definition.

.. code:: c++

    #include "quill/Utility.h"

    QUILL_DEFINE_SEQUENTIAL_THREAD_ID

The C++20 module supplies its own counter and cache. On Linux, an executable and a shared library linked
at startup can share both through normal dynamic symbol resolution when each links the static module.

.. code:: cmake

    add_compile_definitions(QUILL_ENABLE_ASSERTIONS)

Enable internal assertions in release builds. By default they run only when ``NDEBUG`` is undefined.

.. code:: cmake

    add_compile_definitions(QUILL_DISABLE_NON_PREFIXED_MACROS)

Keep only ``QUILL_LOG_<LEVEL>`` macros to avoid name collisions.

.. code:: cmake

    add_compile_definitions(QUILL_DISABLE_FUNCTION_NAME)

Omit ``__FUNCTION__`` if the output pattern omits ``%(caller_function)``. This also avoids Clang-Tidy warnings
for log calls in lambdas.

.. code:: cmake

    add_compile_definitions(QUILL_DISABLE_FILE_INFO)

Omit ``__FILE__`` and ``__LINE__`` if you do not use ``%(file_name)``, ``%(line_number)``, ``%(short_source_location)``
or ``%(source_location)``.

.. code:: cmake

    add_compile_definitions(QUILL_DETAILED_FUNCTION_NAME)

Use ``__PRETTY_FUNCTION__`` (GCC/Clang) or ``__FUNCSIG__`` (MSVC) for ``%(caller_function)``. See
:ref:`formatters:advanced pattern formatter configuration` for ``PatternFormatterOptions::process_function_name``.

**Immediate flush.** CSV and signal-handler messages follow their logger's flush policy. Internal control
events never trigger it. The following example compiles out the check, making runtime ``set_immediate_flush()``
calls ineffective. See :ref:`overview:synchronized logs for debugging` for runtime and explicit flushing.

.. literalinclude:: snippets/quill_docs_immediate_flush.cpp
   :language: cpp
   :linenos:

.. code:: cmake

    add_compile_definitions(QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_<ACTIVE_LEVEL>)

Set the lowest compiled severity. For example, keep Warning and above.

.. code:: cmake

    add_compile_definitions(QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)

Hiding File Names and Functions From Build Binaries
---------------------------------------------------
Use both ``-DQUILL_DISABLE_FUNCTION_NAME`` and ``-DQUILL_DISABLE_FILE_INFO`` to keep Quill call sites from
embedding function signatures and source paths that reveal your codebase layout.

LOGV Macros
-----------
``LOGV_*`` logs l-values with their names. It builds a fixed format for up to 26 arguments. Use ``LOG_*`` for
{fmt} placeholders. Avoid brace initialization inside a ``LOGV_*`` argument, as shown below.

.. code:: cpp

    std::string s{"test"};
    LOGV_INFO(logger, "Answer", s);

Outputs:

.. code-block:: text

    Answer [s: test]

**Brace initialization in a macro argument fails**

.. code:: cpp

    LOGV_INFO(logger, "The answer is", std::string{"test"});

**Parenthesized construction works**

.. code:: cpp

    LOGV_INFO(logger, "The answer is", std::string("test"));

Logging Built-in Types and Strings
----------------------------------
Log arithmetic types, ``std::string``, ``std::string_view``, C strings, C character arrays and ``void const*``
without extra codec headers.

.. code:: cpp

    double a = 123.4567;
    LOG_INFO(logger, "number {:.2f}", a);
    LOGV_INFO(logger, "number", a);

Outputs:

.. code-block:: text

    number 123.46
    number [a: 123.4567]

Logging Arithmetic C-style Arrays
---------------------------------
Include ``quill/std/Array.h`` for arithmetic C arrays.

.. code:: cpp

    #include "quill/std/Array.h"

    int a[3] = {1, 2, 3};
    LOG_INFO(logger, "array {}", a);
    LOGV_INFO(logger, "array", a);

Outputs:

.. code-block:: text

    array [1, 2, 3]
    array [a: [1, 2, 3]]

Logging Enums
-------------
Define ``format_as`` to map an enum to a string or its underlying type. You can also specialize
``fmtquill::formatter`` or cast the value to its underlying type.

**Using format_as**

.. code:: cpp

    #include "quill/bundled/fmt/format.h"

    enum class LogColour
    {
      Red,
      Green,
      Blue
    };

    inline auto format_as(LogColour c) -> std::string_view
    {
      switch (c)
      {
      case LogColour::Red:
        return "Red";
      case LogColour::Green:
        return "Green";
      case LogColour::Blue:
        return "Blue";
      }
      return "Unknown";
    }

    LogColour c {LogColour::Green};
    LOG_INFO(logger, "Colour {}", c);
    LOGV_INFO(logger, "Colour", c);

Outputs:

.. code-block:: text

    Colour Green
    Colour [c: Green]

**Alternative: Using ostream_formatter**

.. code:: cpp

    #include "quill/bundled/fmt/format.h"
    #include "quill/bundled/fmt/ostream.h"

    enum class LogColour
    {
      Red,
      Green,
      Blue
    };

    std::ostream& operator<<(std::ostream& os, LogColour c)
    {
      switch (c)
      {
      case LogColour::Red:
        os << "Red";
        break;
      case LogColour::Green:
        os << "Green";
        break;
      case LogColour::Blue:
        os << "Blue";
        break;
      }
      return os;
    }

    template <>
    struct fmtquill::formatter<LogColour> : fmtquill::ostream_formatter
    {
    };

    LogColour c {LogColour::Green};
    LOG_INFO(logger, "Colour {}", c);
    LOGV_INFO(logger, "Colour", c);

Outputs:

.. code-block:: text

    Colour Green
    Colour [c: Green]

Logging Strings Without Additional Copy
---------------------------------------
Use ``quill::utility::StringRef`` to skip the default deep copy for an immutable string that stays valid
until the backend reads it, such as a literal or static string.

.. code:: cpp

    #include "quill/StringRef.h"

    static constexpr std::string_view s {"Test String"};
    LOG_INFO(logger, "The answer is {}", quill::utility::StringRef {s});

    auto sref = quill::utility::StringRef {s};
    LOGV_INFO(logger, "The answer is", sref);

Outputs:

.. code-block:: text

    The answer is Test String
    The answer is [sref: Test String]

Logging STL Library Types
-------------------------
Include the matching ``quill/std/`` header. Quill supplies codecs for most standard containers.

.. note::

   The backend builds a temporary container with default-constructed allocators, comparators, hash and
   equality functions. Their original state is discarded. They must accept decoded elements, e.g.
   ``std::string_view`` for ``std::string`` keys. Quill rebinds ``std::less<Key>`` and ``std::greater<Key>``
   to the decoded key type.

.. code:: cpp

    #include "quill/std/Vector.h"

    std::vector<std::string> v1 {"One", "Two", "Three"};
    std::vector<std::string> v2 {"Four", "Five", "Six"};
    std::vector<std::vector<std::string>> vv {v1, v2};

    LOG_INFO(logger, "Two vectors {} {} and a vector of vectors {}", v1, v2, vv);
    LOGV_INFO(logger, "Two vectors and a vector of vectors", v1, v2, vv);

Outputs:

.. code-block:: text

    Two vectors ["One", "Two", "Three"] ["Four", "Five", "Six"] and a vector of vectors [["One", "Two", "Three"], ["Four", "Five", "Six"]]
    Two vectors and a vector of vectors [v1: ["One", "Two", "Three"], v2: ["Four", "Five", "Six"], vv: [["One", "Two", "Three"], ["Four", "Five", "Six"]]]

Logging Atomic Values
---------------------
Include ``quill/std/Atomic.h`` for ``std::atomic<T>`` with arithmetic ``T`` (bool, integer or floating point).
The caller takes a relaxed load and the formatter prints the underlying value.

.. code:: cpp

    #include "quill/std/Atomic.h"

    std::atomic<unsigned> counter{42};
    LOG_INFO(logger, "Count {}", counter);

Arrays of atomics are currently unsupported.

Logging Nested STL Library Types
--------------------------------
Include each nested type's codec. A vector of pairs needs both ``quill/std/Vector.h`` and ``quill/std/Pair.h``.

.. code:: cpp

    #include "quill/std/Vector.h"
    #include "quill/std/Pair.h"

    std::vector<std::pair<int, std::string>> v1 {{1, "One"}, {2, "Two"}, {3, "Three"}};
    LOG_INFO(logger, "Vector {}", v1);
    LOGV_INFO(logger, "Vector", v1);

Outputs:

.. code-block:: text

    Vector [(1, "One"), (2, "Two"), (3, "Three")]
    Vector [v1: [(1, "One"), (2, "Two"), (3, "Three")]]

The same rule applies at any nesting depth.

.. code:: cpp

    #include "quill/std/Chrono.h"
    #include "quill/std/Optional.h"
    #include "quill/std/Pair.h"
    #include "quill/std/Vector.h"

    std::vector<std::pair<std::chrono::system_clock::time_point, std::optional<std::string>>> v1{
      {std::chrono::system_clock::now(), "One"}, {std::chrono::system_clock::now(), "Two"}};

    LOG_INFO(logger, "Vector {}", v1);
    LOGV_INFO(logger, "Vector", v1);

Outputs:

.. code-block:: text

    Vector [(2024-07-27 10:14:32.851648339, optional("One")), (2024-07-27 10:14:32.851648405, optional("Two"))]
    Vector [v1: [(2024-07-27 10:14:32.851648339, optional("One")), (2024-07-27 10:14:32.851648405, optional("Two"))]]

Logging Wide Strings
--------------------
On Windows, include ``quill/std/WideString.h``. See :doc:`wide_strings` for output and container support.

.. code:: cpp

    #include "quill/std/WideString.h"
    #include "quill/std/Vector.h"

    std::wstring w {L"wide"};
    std::vector<std::wstring> wv {L"wide", L"string"};
    LOG_INFO(logger, "string {} and vector {}", w, wv);
    LOGV_INFO(logger, "string and vector", w, wv);

Outputs:

.. code-block:: text

    string wide and vector ["wide", "string"]
    string and vector [w: wide, wv: ["wide", "string"]]

Logging std::variant, std::bitset, std::complex, std::error_code
----------------------------------------------------------------
Include the matching ``quill/std/`` header for these non-container types.

.. code:: cpp

    #include "quill/std/Variant.h"
    #include "quill/std/Bitset.h"
    #include "quill/std/Complex.h"
    #include "quill/std/SystemError.h"

    std::variant<int, std::string> v{"active"};
    std::bitset<8> flags{0b10110010};
    std::complex<double> z{1.5, -2.0};
    std::error_code ec = std::make_error_code(std::errc::timed_out);

    LOG_INFO(logger, "variant {} bitset {} complex {} error {}", v, flags, z, ec);

Their codecs also work inside supported containers, e.g. ``std::vector<std::variant<...>>``.

Logging std::chrono Types
-------------------------
``quill/std/Chrono.h`` supports ``std::chrono::system_clock::time_point``, ``std::chrono::duration`` and
``std::tm``. Use {fmt} chrono specifiers such as ``{:%Y-%m-%d %H:%M:%S}`` and ``{:%Q %q}``.

C++20 libraries defining ``__cpp_lib_chrono >= 201907L`` also support ``std::chrono::year``, ``month``, ``day``,
``weekday`` and ``year_month_day``. Libraries reporting an older value, including GCC 13's libstdc++, lack
the bundled {fmt} formatters for those calendar types. Other chrono types, including
``std::chrono::steady_clock`` time points, need their own support.

.. code:: cpp

    #include "quill/std/Chrono.h"

    auto now = std::chrono::system_clock::now();
    LOG_INFO(logger, "now is {:%Y-%m-%d %H:%M:%S}", now);

Logging User Defined Types
--------------------------
Choose how to format the type and how to carry it to the backend. The formatter describes its text.
The codec determines which data the originating thread must copy or format.

Quick Decision Guide
~~~~~~~~~~~~~~~~~~~~

- **Deferred.** Use ``DeferredFormatCodec<T>`` for objects safe to transfer across threads. This includes
  trivial types, copy-only types and move-only types passed as temporaries or with ``std::move()``.
  Non-trivial types need a copy or move constructor. Copied members must stay safe to read, including
  state reached through a ``std::shared_ptr``.
- **Direct.** Use ``DirectFormatCodec<T>`` to format once on the caller with {fmt}, for example when raw
  pointers, references or non-copyable resources make deferred access unsafe. This costs caller time but
  captures the text even if the source state later changes.
- **Custom.** Specialize ``quill::Codec<T>`` to encode selected fields or choose a binary layout.
- **Shared formatting.** Use ``format_as(T)`` to share formatting with standalone ``fmt``.

Logging Requirements
~~~~~~~~~~~~~~~~~~~~

Provide a ``quill::Codec<T>`` specialization (directly or through a helper) and either a
``fmtquill::formatter<T>`` specialization in ``fmtquill`` or a discoverable ``format_as(T)``.

Codec Exception Contract
~~~~~~~~~~~~~~~~~~~~~~~~

``compute_encoded_size()`` may throw before queue reservation. ``encode()`` must complete without throwing.
Consider two deferred arguments.

.. code-block:: cpp

    LOG_INFO(logger, "{} {}", first, second);

If the first copy succeeds and the second throws, the record is incomplete.

.. code-block:: text

    first:  constructed but never destroyed
    second: construction failed
    record: never committed

The unpublished record never reaches the backend and the queue stays usable. The first copy's destructor
does not run, so it can retain resources. Constructors and destructors used by ``DeferredFormatCodec`` must
return normally. ``DirectFormatCodec`` formats during size calculation, so a throwing formatter fails the
call before anything is queued.

A custom decoder must consume its entire payload without throwing. Records carry no byte length for
skipping a failed decoder. Catch recoverable errors and supply a fallback value where possible.

If a decoder throws (e.g. allocating a ``std::vector`` fails), the backend retries the same record. A top-level
deferred argument is formatted in place and survives until formatting completes. A deferred element already
moved out of a ``std::vector``, ``std::optional``, ``std::pair``, ``std::tuple`` or ``std::variant`` has been
destroyed in the queue. A retry would read that destroyed object.

No Logging Inside a Codec
~~~~~~~~~~~~~~~~~~~~~~~~~

Never log through Quill inside ``compute_encoded_size()``, ``encode()``, a ``DirectFormatCodec`` formatter,
or a ``DeferredFormatCodec`` copy/move constructor. Even a different logger uses the same thread's queue
and cached sizes. A nested call can corrupt the record being written.

Logging User-Defined Types in STL Containers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For ``std::vector<UserType>`` or another supported container, provide the element codec and formatter, then
include the matching container header from ``quill/std/``.

DeferredFormatCodec
~~~~~~~~~~~~~~~~~~~

Basic Example
^^^^^^^^^^^^^

.. literalinclude:: snippets/quill_docs_deferred_format_codec.cpp
   :language: cpp
   :linenos:

Outputs:

.. code-block:: text

    User is [Name: Super, Surname: User, Age: 1, Favorite Colors: ["red", "blue", "green"]]

Serialising Trivially Copyable Types With Non-Default Constructor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: snippets/quill_docs_trivially_copyable_codec.cpp
   :language: cpp
   :linenos:

Outputs:

.. code-block:: text

    Order is timestamp=17395040124686356 price=220.1 quantity=100

DirectFormatCodec
~~~~~~~~~~~~~~~~~

The caller caches the formatted bytes until it copies them to the queue. Long values can allocate.
Capacity lasts until the thread context is destroyed, so one large record can increase a thread's retained memory.

.. literalinclude:: snippets/quill_docs_direct_format_codec.cpp
   :language: cpp
   :linenos:

Outputs:

.. code-block:: text

    User is [Name: Super, Surname: User, Age: 1, Favorite Colors: ["red", "blue", "green"]]

Writing Custom Codec
~~~~~~~~~~~~~~~~~~~~

Serialising Non Trivially Copyable User Defined Types With Public Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For accessible members of a default-constructible type, list the fields once with ``MemberwiseCodec`` from
``quill/MemberwiseCodec.h``.

.. code:: cpp

    template <>
    struct quill::Codec<Order> : quill::MemberwiseCodec<Order, &Order::symbol, &Order::price, &Order::quantity>
    {
    };

The hand-written equivalent below is a starting point for a custom layout. Include each member type's
``quill/std/`` header before calling ``compute_total_encoded_size``, ``encode_members`` or ``decode_members``.

``decode_members`` assigns decoded values to members. When the decoded type cannot be assigned, decode
it explicitly in encoding order. For example, ``std::vector<std::string>`` decodes to
``std::vector<std::string_view>``, and ``std::variant<int, std::string>`` to ``std::variant<int, std::string_view>``.

.. code:: cpp

    decode_members(buffer, order, order.symbol, order.price);

    auto const tags = quill::Codec<std::vector<std::string>>::decode_arg(buffer); // std::vector<std::string_view>
    order.tags.assign(tags.begin(), tags.end());

.. code:: cpp

    #include "quill/bundled/fmt/ostream.h"
    #include "quill/bundled/fmt/format.h"

    #include "quill/core/Codec.h"
    #include "quill/core/DynamicFormatArgStore.h"

    struct Order
    {
      std::string symbol;
      double price;
      int quantity;

      friend std::ostream& operator<<(std::ostream& os, Order const& order)
      {
        os << "symbol=" << order.symbol << " price=" << order.price << " quantity=" << order.quantity;
        return os;
      }
    };

    template <>
    struct fmtquill::formatter<Order> : fmtquill::ostream_formatter
    {
    };

    template <>
    struct quill::Codec<Order>
    {
      static size_t compute_encoded_size(quill::SizeCacheVector& conditional_arg_size_cache, ::Order const& order) noexcept
      {
        return compute_total_encoded_size(conditional_arg_size_cache, order.symbol, order.price, order.quantity);
      }

      static void encode(std::byte*& buffer, quill::SizeCacheVector const& conditional_arg_size_cache,
                         uint32_t& conditional_arg_size_cache_index, ::Order const& order) noexcept
      {
        encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, order.symbol,
                       order.price, order.quantity);
      }

      static ::Order decode_arg(std::byte*& buffer)
      {
        ::Order order;
        decode_members(buffer, order, order.symbol, order.price, order.quantity);
        return order;
      }

      static void decode_and_store_arg(std::byte*& buffer, DynamicFormatArgStore* args_store)
      {
        args_store->push_back(decode_arg(buffer));
      }
    };

    Order order {"AAPL", 220.10, 100};

    LOG_INFO(logger, "Order is {}", order);
    LOGV_INFO(logger, "Order", order);

Outputs:

.. code-block:: text

    Order is symbol=AAPL price=220.1 quantity=100
    Order [order: symbol=AAPL price=220.1 quantity=100]

Serialising Non Trivially Copyable User Defined Types With Private Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For private non-trivial members, declare ``quill::Codec<T>`` a friend and provide a private default
constructor if necessary. This example includes ``quill/std/Chrono.h`` for its
``std::chrono::system_clock::time_point`` member.

.. literalinclude:: snippets/quill_docs_custom_codec_private_members.cpp
   :language: cpp
   :linenos:

Outputs:

.. code-block:: text

    Order is timestamp=17220432928367021 symbol=AAPL price=220.1 quantity=100
    Order [order: timestamp=17220432928367021 symbol=AAPL price=220.1 quantity=100]

Sharing Formatting With External fmt
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill bundles {fmt} in ``fmtquill``. Put a free ``format_as(T)`` in the same namespace as ``T`` so both
``fmtquill`` and standalone ``fmt`` find it by argument-dependent lookup (ADL).

Preferred: Use ``format_as``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: cpp

    namespace example
    {
    struct User
    {
      int id = 1;
      int age = 32;
    };

    inline auto format_as(User const& user) -> std::string
    {
      return "id: " + std::to_string(user.id) + ", age: " + std::to_string(user.age);
    }
    } // namespace example

    template <>
    struct quill::Codec<example::User> : quill::DeferredFormatCodec<example::User>
    {
    };

    // Works with Quill:
    // LOG_INFO(logger, "{}", example::User{});
    //
    // The same format_as() is also what standalone fmt would use:
    // fmt::format("{}", example::User{});

If you need a real formatter specialization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If customization requires a formatter, provide separate ``fmtquill::formatter<T>`` and ``fmt::formatter<T>``
specializations with one shared body. Cross-library formatter inheritance can couple incompatible types.

.. code:: cpp

    struct User
    {
      int x = 42;
    };

    struct UserFormatterBase
    {
      template <typename ParseContext>
      constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

      template <typename FormatContext>
      auto format(User const& user, FormatContext& ctx) const -> decltype(ctx.out())
      {
        return fmtquill::format_to(ctx.out(), "x={}", user.x);
      }
    };

    template <>
    struct fmtquill::formatter<User> : UserFormatterBase
    {
    };

    template <>
    struct fmt::formatter<User> : UserFormatterBase
    {
    };

    template <>
    struct quill::Codec<User> : quill::DeferredFormatCodec<User>
    {
    };

For ``ostream_formatter``, keep one specialization per namespace.

.. code:: cpp

    template <>
    struct fmt::formatter<User> : fmt::ostream_formatter
    {
    };

    template <>
    struct fmtquill::formatter<User> : fmtquill::ostream_formatter
    {
    };

    template <>
    struct quill::Codec<User> : quill::DeferredFormatCodec<User>
    {
    };

Helper Macros for Logging User Defined Types
--------------------------------------------
Include ``quill/HelperMacros.h`` to generate a codec specialization for a streamable type.

.. code:: cpp

    #include "quill/HelperMacros.h"

    // For types containing pointers or other unsafe members
    QUILL_LOGGABLE_DIRECT_FORMAT(Type);

    // For types that only contain value types and are safe to copy
    QUILL_LOGGABLE_DEFERRED_FORMAT(Type);

- ``QUILL_LOGGABLE_DIRECT_FORMAT`` formats immediately for pointer members or other lifetime dependencies.
- ``QUILL_LOGGABLE_DEFERRED_FORMAT`` copies a value-only type for backend formatting.

.. code:: cpp

    class User
    {
    public:
      std::string name;
      uint64_t* value_ptr{nullptr};  // Contains a pointer - unsafe to copy
    };

    std::ostream& operator<<(std::ostream& os, User const& user)
    {
      os << "User(name: " << user.name << ", value: " << (user.value_ptr ? *user.value_ptr : 0) << ")";
      return os;
    }

    // Mark as unsafe type - will be formatted immediately
    QUILL_LOGGABLE_DIRECT_FORMAT(User)

    class Product
    {
    public:
      std::string name;      // Only contains value types
      double price{0.0};     // Safe to copy across threads
      int quantity{0};
    };

    std::ostream& operator<<(std::ostream& os, Product const& product)
    {
      os << "Product(name: " << product.name << ", price: $" << product.price
         << ", quantity: " << product.quantity << ")";
      return os;
    }

    // Mark as safe type - can be formatted asynchronously
    QUILL_LOGGABLE_DEFERRED_FORMAT(Product)

Both helpers require an ``operator<<`` for the type.
