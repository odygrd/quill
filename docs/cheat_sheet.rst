.. title:: Cheat Sheet

.. _cheat_sheet:

Cheat Sheet
===========

Introduction
------------
The logging library provides macros to efficiently log various types of data. The ``LOG_`` macro copies each argument passed into contiguous pre-allocated memory. On the invoking thread, only a binary copy of the arguments is taken. Formatting, string conversions, and IO are handled by the backend logging thread.

For instance, when logging a ``std::vector``, no copy constructor is invoked. Instead, the vector elements are serialised into pre-allocated memory.

Quill uses ``libfmt`` for formatting, supporting the ``{}`` placeholders syntax (https://fmt.dev/latest/syntax).

The library minimizes unnecessary header inclusions; therefore, to log STL types, you must explicitly include headers from the ``quill/std/`` folder.

Preprocessor Configuration Flags
--------------------------------
The library provides several preprocessor flags to customize its behavior at compile time.

.. code:: cmake

    add_compile_definitions(-DQUILL_NO_EXCEPTIONS)

Disables exception handling support, allowing the library to be built without exceptions.

.. code:: cmake

    add_compile_definitions(-DQUILL_NO_THREAD_NAME_SUPPORT)

Disables features that require thread name retrieval. This is useful for compatibility with older Windows versions (e.g., Windows Server 2012/2016) and Android.

.. code:: cmake

    add_compile_definitions(-DQUILL_ENABLE_ASSERTIONS)

Enables internal assertions in release builds. By default, assertions are active only in debug builds ``(!defined(NDEBUG))``. Defining this flag forces them on even in release mode, which can help catch issues at runtime.

.. code:: cmake

    add_compile_definitions(-DQUILL_DISABLE_NON_PREFIXED_MACROS)

Disables the non-prefixed `LOG_<LEVEL>` macros, keeping only the `QUILL_LOG_<LEVEL>` macros. This helps prevent conflicts with other logging libraries.

.. code:: cmake

    add_compile_definitions(-DQUILL_DISABLE_FUNCTION_NAME)

Disables ``__FUNCTION__`` information in log statements at compile time when the function-related pattern (``%(caller_function)``) is not needed in the ``PatternFormatter``. This can also be used to eliminate Clang-Tidy warnings when using log statements inside lambda expressions.

.. code:: cmake

    add_compile_definitions(-DQUILL_DISABLE_FILE_INFO)

Disables ``__FILE__`` and ``__LINE__`` information in log statements at compile time when location-related patterns (``%(file_name)``, ``%(line_number)``, ``%(short_source_location)``, ``%(source_location)``) are not needed in the ``PatternFormatter``. This removes embedded source path strings from built binaries from the security viewpoint.

.. code:: cmake

    option(QUILL_DETAILED_FUNCTION_NAME "Use detailed function name (__PRETTY_FUNCTION__ or __FUNCSIG__) instead of __FUNCTION__ in LOG_* macros" OFF)

Enables the use of compiler-specific detailed function signatures (such as ``__PRETTY_FUNCTION__`` on GCC/Clang or ``__FUNCSIG__`` on MSVC) instead of the standard ``__FUNCTION__`` in log macros. This option is only relevant when ``%(caller_function)`` is used in the pattern formatter. When enabled, you can further customize the function name display by providing a processing function via ``PatternFormatterOptions::process_function_name``.

.. code:: cmake

    add_compile_definitions(-DQUILL_IMMEDIATE_FLUSH=0)

Immediate flushing blocks the calling thread until a log message has been written to its destination, effectively simulating synchronous logging.
This feature can be enabled at runtime on a ``Logger`` instance by calling ``logger->set_immediate_flush(1)``.
Setting ``QUILL_ENABLE_IMMEDIATE_FLUSH=0`` in the preprocessor disables this feature completely, eliminating the conditional branch from the hot path and improving performance.
When disabled at compile time, ``logger->set_immediate_flush(flush_every_n_messages)`` will have no effect.

.. code:: cmake

    add_compile_definitions(-DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_<ACTIVE_LEVEL>)

Compiles only the specified log level and higher, excluding lower levels at compile time. This helps reduce branching in optimized builds.
For example, to keep only warning level and above:

.. code:: cmake

    add_compile_definitions(-DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)

Hiding File Names and Functions From Build Binaries
--------------------------------------------------
From a security standpoint, embedded source file paths and function signatures in binaries can leak sensitive information about your codebase structure.
To protect use both the `-DQUILL_DISABLE_FUNCTION_NAME` and `-DQUILL_DISABLE_FILE_INFO` compile definitions described above.
When both options are enabled, neither function names nor file paths will be embedded in your binary, significantly reducing the information available to anyone examining the compiled code.

LOGV Macros
-----------
In addition to the ``LOG_`` macros, the ``LOGV_`` macros provide a convenient alternative.

However, it's important to note that these macros do not support the ``libfmt`` syntax. Instead, they use a hardcoded format string and support up to 20 arguments.

Please avoid using ``{}`` placeholders with these macros, such as brace initialization, as they are primarily intended for logging l-values.

**Recommended Usage:**

.. code:: cpp

    std::string s{"test"};
    LOGV_INFO(logger, "Answer", s);

Outputs:

    Answer [s: test]

**This will NOT work:**

.. code:: cpp

    LOGV_INFO(logger, "The answer is", std::string{"test"});

**But this is fine:**

.. code:: cpp

    LOGV_INFO(logger, "The answer is", std::string("test"));

Logging Built-in Types and Strings
----------------------------------
Logging ``arithmetic types``, ``strings``, ``string_view``, ``C strings``, ``C char arrays``, or ``void const*`` is supported out of the box.

.. code:: cpp

    double a = 123.4567;
    LOG_INFO(logger, "number {:.2f}", a);
    LOGV_INFO(logger, "number", a);

Outputs:

    number 123.46

    number [a: 123.4567]

Logging Arithmetic C-style Arrays
---------------------------------
This functionality is supported by including ``quill/std/Array.h``

.. code:: cpp

    #include "quill/std/Array.h"

    int a[3] = {1, 2, 3};
    LOG_INFO(logger, "array {}", a);
    LOGV_INFO(logger, "array", a);

Outputs:

    array [1, 2, 3]

    array [a: [1, 2, 3]]

Logging Enums
-------------
For enums, you can either cast them to their underlying type or provide an ``operator<<`` or an ``fmtquill::formatter``

.. code:: cpp

    #include "quill/bundled/fmt/format.h"
    #include "quill/bundled/fmt/ostream.h"

    enum class Side
    {
      BUY,
      SELL
    };

    std::ostream& operator<<(std::ostream& os, Side s)
    {
      if (s == Side::BUY)
      {
        os << "BUY";
      }
      else
      {
        os << "SELL";
      }
      return os;
    }

    template <>
    struct fmtquill::formatter<Side> : fmtquill::ostream_formatter
    {

    };

    Side s {Side::SELL};
    LOG_INFO(logger, "Side {}", s);
    LOGV_INFO(logger, "Side", s);

Outputs:

    Side SELL

    Side [s: SELL]

Logging Strings Without Additional Copy
---------------------------------------
By default, the logger takes a deep copy of any string for thread safety. To log an immutable string with a valid lifetime without copying (e.g., string literals, static strings), use ``quill::utility::StringRef``.

.. code:: cpp

    #include "quill/StringRef.h"

    static constexpr std::string_view s {"Test String"};
    LOG_INFO(logger, "The answer is {}", quill::utility::StringRef {s});

    auto sref = quill::utility::StringRef {s};
    LOGV_INFO(logger, "The answer is", sref);

Outputs:

    The answer is Test String

    The answer is [sref: Test String]

Logging STL Library Types
-------------------------
To log STL types, include the relevant header from ``quill/std/``. There is support for most ``STL`` types.

.. code:: cpp

    #include "quill/std/Vector.h"

    std::vector<std::string> v1 {"One", "Two", "Three"};
    std::vector<std::string> v2 {"Four", "Five", "Six"};
    std::vector<std::vector<std::string>> vv {v1, v2};

    LOG_INFO(logger, "Two vectors {} {} and a vector of vectors {}", v1, v2, vv);
    LOGV_INFO(logger, "Two vectors and a vector of vectors", v1, v2, vv);

Outputs:

    Two vectors ["One", "Two", "Three"] ["Four", "Five", "Six"] and a vector of vectors [["One", "Two", "Three"], ["Four", "Five", "Six"]]

    Two vectors and a vector of vectors [v1: ["One", "Two", "Three"], v2: ["Four", "Five", "Six"], vv: [["One", "Two", "Three"], ["Four", "Five", "Six"]]]

Logging Nested STL Library Types
--------------------------------
Logging nested STL types is supported. Include all relevant files from ``quill/std/``.

For example, to log a ``std::vector`` of ``std::pair``, include both ``quill/std/Vector.h`` and ``quill/std/Pair.h``.

.. code:: cpp

    #include "quill/std/Vector.h"
    #include "quill/std/Pair.h"

    std::vector<std::pair<int, std::string>> v1 {{1, "One"}, {2, "Two"}, {3, "Three"}};
    LOG_INFO(logger, "Vector {}", v1);
    LOGV_INFO(logger, "Vector", v1);

Outputs:

    Vector [(1, "One"), (2, "Two"), (3, "Three")]

    Vector [v1: [(1, "One"), (2, "Two"), (3, "Three")]]

You can have multiple levels of nested types without limitation. As long as all relevant headers are included, the serialization will work seamlessly.

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

    Vector [(2024-07-27 10:14:32.851648339, optional("One")), (2024-07-27 10:14:32.851648405, optional("Two"))]

    Vector [v1: [(2024-07-27 10:14:32.851648339, optional("One")), (2024-07-27 10:14:32.851648405, optional("Two"))]]

Logging Wide Strings
--------------------
On Windows, wide strings are supported by including ``quill/std/WideString.h``. For more information see the Wide Strings tutorial section

.. code:: cpp

    #include "quill/std/WideString.h"
    #include "quill/std/Vector.h"

    std::wstring w {L"wide"};
    std::vector<std::wstring> wv {L"wide", L"string"};
    LOG_INFO(logger, "string {} and vector {}", w, wv);
    LOGV_INFO(logger, "string and vector", w, wv);

Outputs:

    string wide and vector ["wide", "string"]

    string and vector [w: wide, wv: ["wide", "string"]]

Logging User Defined Types
--------------------------
To log user-defined types, you need to define how they should be serialized or converted to a string before passing them to the logger. There are several ways to achieve this:

    1. **Use DeferredFormatCodec**
       If the object is safe to copy across threads (e.g., does not contain `std::shared_ptr` members being modified), this approach takes a copy of the object and formats it later on the backend logging thread.

       - Works for both trivially and non-trivially copyable types.
       - If the type is **not trivially copyable**, it requires either a **move constructor** or a **copy constructor** (or both).
       - **Move-only types** (with deleted copy constructors) are supported. Pass them using rvalue references (``std::move()`` or temporaries).
       - **Copy-only types** (with deleted move constructors) are also supported.

    2. **Use DirectFormatCodec**
       Suitable for objects that are not safe to copy across threads (e.g., contain raw pointers, references, or non-copyable resources). This method converts the object to a string immediately in the hot path using `fmt::format`, which increases hot-path latency.

    3. **Implement a Custom Codec**
       For maximum flexibility, you can define a custom codec to specify exactly how the object should be serialized and deserialized.

Logging Requirements
~~~~~~~~~~~~~~~~~~~~

To ensure a user-defined type can be logged, you must:

    - Specialize ``quill::Codec<T>`` for your type.
    - Specialize ``fmtquill::formatter<T>`` under the ``fmtquill`` namespace.

Logging User-Defined Types in STL Containers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

User-defined types nested within STL containers, such as ``std::vector<UserType>``, can also be logged. To ensure proper serialization, you must:

    1. Follow one of the three approaches above.
    2. Include the relevant STL type header from the ``quill/std/`` directory.

DeferredFormatCodec
~~~~~~~~~~~~~~~~~~~

Basic Example
^^^^^^^^^^^^^

.. code:: cpp

    #include "quill/DeferredFormatCodec.h"

    class User
    {
    public:
      User(std::string name, std::string surname, uint32_t age)
        : name(std::move(name)), surname(std::move(surname)), age(age)
      {
        favorite_colors.push_back("red");
        favorite_colors.push_back("blue");
        favorite_colors.push_back("green");
      };

      std::string name;
      std::string surname;
      uint32_t age{};
      std::vector<std::string> favorite_colors;
    };

    /***/
    template <>
    struct fmtquill::formatter<User>
    {
      constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

      auto format(::User const& user, format_context& ctx) const
      {
        return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
                                   user.name, user.surname, user.age, user.favorite_colors);
      }
    };

    /***/
    template <>
    struct quill::Codec<User> : quill::DeferredFormatCodec<User>
    {
    };

    User user{"Super", "User", 1};
    LOG_INFO(logger, "User is [{}]", user);

Outputs:

    User is [Name: Super, Surname: User, Age: 1, Favorite Colors: ["red", "blue", "green"]]

Serialising Trivially Copyable Types With Non-Default Constructor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: cpp

    #include "quill/DeferredFormatCodec.h"

    class Order
    {
    public:
      Order(double price, int quantity)
        : timestamp(std::chrono::system_clock::now().time_since_epoch().count()), price(price), quantity(quantity)
      {
      }

    private:
      uint64_t timestamp;
      double price;
      int quantity;

      friend struct quill::DeferredFormatCodec<Order>;

      Order() = default;

      friend std::ostream& operator<<(std::ostream& os, Order const& order)
      {
        os << "timestamp=" << order.timestamp << " price=" << order.price << " quantity=" << order.quantity;
        return os;
      }
    };

    template <>
    struct fmtquill::formatter<Order> : fmtquill::ostream_formatter
    {
    };

    template <>
    struct quill::Codec<Order> : quill::DeferredFormatCodec<Order>
    {
    };

    Order order {220.10, 100};
    LOG_INFO(logger, "Order is {}", order);

Outputs:

    Order is timestamp=17395040124686356 price=220.1 quantity=100

DirectFormatCodec
~~~~~~~~~~~~~~~~~

.. code:: cpp

    #include "quill/DirectFormatCodec.h"

    class User
    {
    public:
      User(std::string name, std::string surname, uint32_t age)
        : name(std::move(name)), surname(std::move(surname)), age(age)
      {
        favorite_colors.push_back("red");
        favorite_colors.push_back("blue");
        favorite_colors.push_back("green");
      };

      std::string name;
      std::string surname;
      uint32_t age{};
      std::vector<std::string> favorite_colors;
    };

    /***/
    template <>
    struct fmtquill::formatter<User>
    {
      constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

      auto format(::User const& user, format_context& ctx) const
      {
        return fmtquill::format_to(ctx.out(), "Name: {}, Surname: {}, Age: {}, Favorite Colors: {}",
                                   user.name, user.surname, user.age, user.favorite_colors);
      }
    };

    /***/
    template <>
    struct quill::Codec<User> : quill::DirectFormatCodec<User>
    {
    };

    User user{"Super", "User", 1};
    LOG_INFO(logger, "User is [{}]", user);

Outputs:

    User is [Name: Super, Surname: User, Age: 1, Favorite Colors: ["red", "blue", "green"]]

Writing Custom Codec
~~~~~~~~~~~~~~~~~~~~

Serialising Non Trivially Copyable User Defined Types With Public Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that STL types can be used in custom codecs by passing them to ``compute_total_encoded_size``, ``encode_members``, and ``decode_members``, provided you include the relevant header from ``quill/std/`` for each STL type used.

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
      static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache, ::Order const& order) noexcept
      {
        return compute_total_encoded_size(conditional_arg_size_cache, order.symbol, order.price, order.quantity);
      }

      static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
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

    Order is symbol=AAPL price=220.1 quantity=100

    Order [order: symbol=AAPL price=220.1 quantity=100]

Serialising Non Trivially Copyable User Defined Types With Private Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For user-defined types with non-trivially copyable types as private members, the easiest workaround is the same as in the trivially copyable case above: make ``quill::Codec<T>`` a friend and also have a private default constructor if the default one is not publicly available.

Note that it is possible to pass STL types to ``compute_total_encoded_size``, ``encode_members``, and ``decode_members`` as long as the relevant header file from ``quill/std/`` for that type is included. In this example, ``quill/std/Chrono.h`` is included to ``encode`` and ``decode`` the ``std::chrono::system_clock::time_point``.

.. code:: cpp

    #include "quill/bundled/fmt/ostream.h"
    #include "quill/bundled/fmt/format.h"

    #include "quill/core/Codec.h"
    #include "quill/core/DynamicFormatArgStore.h"
    #include "quill/std/Chrono.h"

    class Order
    {
    public:
      Order(std::string symbol, double price, int quantity)
        : timestamp(std::chrono::system_clock::now().time_since_epoch().count()), symbol(std::move(symbol)), price(price), quantity(quantity)
      {
      }

    private:
      std::chrono::system_clock::time_point timestamp;
      std::string symbol;
      double price;
      int quantity;

      template <typename T, typename U>
      friend struct quill::Codec;

      Order() = default;

      friend std::ostream& operator<<(std::ostream& os, Order const& order)
      {
        os << "timestamp=" << order.timestamp.time_since_epoch().count() << " symbol=" << order.symbol << " price=" << order.price << " quantity=" << order.quantity;
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
      static size_t compute_encoded_size(detail::SizeCacheVector& conditional_arg_size_cache, ::Order const& order) noexcept
      {
        return compute_total_encoded_size(conditional_arg_size_cache, order.timestamp, order.symbol, order.price, order.quantity);
      }

      static void encode(std::byte*& buffer, detail::SizeCacheVector const& conditional_arg_size_cache,
                         uint32_t& conditional_arg_size_cache_index, ::Order const& order) noexcept
      {
        encode_members(buffer, conditional_arg_size_cache, conditional_arg_size_cache_index, order.timestamp, order.symbol,
                       order.price, order.quantity);
      }

      static ::Order decode_arg(std::byte*& buffer)
      {
        ::Order order;
        decode_members(buffer, order, order.timestamp, order.symbol, order.price, order.quantity);
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

    Order is timestamp=17220432928367021 symbol=AAPL price=220.1 quantity=100

    Order [order: timestamp=17220432928367021 symbol=AAPL price=220.1 quantity=100]

Using External fmt Formatter Specializations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill uses a custom namespace, ``fmtquill``, and requires formatter specializations to be defined under the same namespace. However, when an external ``libfmt`` is also used, you can reuse existing ``fmt::formatter`` specializations instead of redefining them.

.. note::

   Ensure that the major version of your external ``libfmt`` matches Quill's internal version to avoid ABI incompatibilities.

If you choose to reuse an existing ``fmt::formatter`` specialization, you can derive from it. However, you must template both ``parse`` and ``format`` to support different ``Context`` types.

.. code:: cpp

    struct User
    {
      int id = 1;
      int age = 32;
    };

    template <>
    struct fmt::formatter<User>
    {
      template <typename TFormatParseCtx>
      constexpr auto parse(TFormatParseCtx& ctx) { return ctx.begin(); }

      template <typename TFormatCtx>
      auto format(::User const& user, TFormatCtx& ctx) const
      {
        return fmt::format_to(ctx.out(), "id: {}, age: {}", user.id, user.age);
      }
    };

    template <>
    struct fmtquill::formatter<User> : fmt::formatter<User>
    {
    };

    template <>
    struct quill::Codec<User> : DeferredFormatCodec<User>
    {
    };

If the external specialization derives from ``fmt::ostream_formatter``, the above approach won't work because ``parse`` is not templated. In this case, you must directly specialize ``fmtquill::ostream_formatter``.

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
    struct quill::Codec<User> : DeferredFormatCodec<User>
    {
    };

Helper Macros for Logging User Defined Types
--------------------------------------------
The library provides helper macros to simplify logging of user-defined types, available by including ``quill/HelperMacros.h``.

.. code:: cpp

    #include "quill/HelperMacros.h"

    // For types containing pointers or other unsafe members
    QUILL_LOGGABLE_DIRECT_FORMAT(Type);

    // For types that only contain value types and are safe to copy
    QUILL_LOGGABLE_DEFERRED_FORMAT(Type);

These macros automatically generate the necessary codec specializations for your user-defined types, making it easier to log custom classes:

**1. QUILL_LOGGABLE_DIRECT_FORMAT**
   Use for types that contain pointer members or have lifetime dependencies that make them unsafe to copy across threads.
   This macro sets up a DirectFormatCodec that formats the object immediately when the log statement is called.

**2. QUILL_LOGGABLE_DEFERRED_FORMAT**
   Use for types that only contain value types (no pointers) and are safe to copy.
   This macro sets up a DeferredFormatCodec that allows the object to be copied and formatted later by the backend thread.

Example:

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

Note that using these macros requires you to provide an ``operator<<`` for your type.
