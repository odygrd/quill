.. title:: Cheat Sheet

Cheat Sheet
===========

Introduction
------------
The logging library provides macros to efficiently log various types of data. The ``LOG_`` macro copies each argument passed into contiguous pre-allocated memory. On the invoking thread, only a binary copy of the arguments is taken. Formatting, string conversions, and IO are handled by the backend logging thread.

For instance, when logging a ``std::vector``, no copy constructor is invoked. Instead, the vector elements are serialised into pre-allocated memory.

Quill uses ``libfmt`` for formatting, supporting the ``{}`` placeholders syntax (https://fmt.dev/latest/syntax).

The library minimizes unnecessary header inclusions; therefore, to log STL types, you must explicitly include headers from the ``quill/std/`` folder.

LOGV Macros
------------
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
--------------------------
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
By default, the logger takes a deep copy of any string. To log an immutable string with a valid lifetime without copying, use ``quill::utility::StringRef``.

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
To log user-defined types, you need to specify how to serialise them or convert them to a string and pass that string to the logger.

Slow Path Logging
~~~~~~~~~~~~~~~~~
For log statements made during program initialization, or for debug logs that are not on the critical path, it is recommended to convert user-defined types to strings and pass these strings to the ``LOG_`` function. This method requires less effort and minimizes template instantiations. For example:

.. code:: cpp

    #include "quill/bundled/fmt/ostream.h"
    #include "quill/bundled/fmt/format.h"

    class Config
    {
      public:
      std::string param_1;
      std::string param_2;

      friend std::ostream& operator<<(std::ostream& os, Config config)
      {
        os << "param_1: " << config.param_1 << " param_2 " << config.param_2;
        return os;
      }
    };

    template <>
    struct fmtquill::formatter<Config> : fmtquill::ostream_formatter
    {
    };

    Config cfg {"123", "456"};

    LOG_INFO(logger, "Starting with config {}", fmtquill::format("{}", cfg));

    std::string const cfg_str = fmtquill::format("{}", cfg);
    LOGV_INFO(logger, "Starting", cfg_str);

Outputs:

    Starting with config param_1: 123 param_2 456

    Starting [cfg_str: param_1: 123 param_2 456]

Hot Path Logging
~~~~~~~~~~~~~~~~~
For log statements on the critical path, it is advisable to provide serialisation methods so that only a binary copy is made during the critical path operations. The type will be encoded on the critical path, then decoded and reconstructed on the backend thread before being passed to ``libfmt`` for formatting. To serialise user defined types types, the library requires:

  1. Template specializations of ``quill::Codec<T>`` within the ``quill`` namespace.
  2. Template specializations of ``fmtquill::formatter<T>` within the ``fmtquill`` namespace.
  3. The user-defined type must have a default constructor and a copy constructor.

Serialising Trivially Copyable Types With Default Constructor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Use the ``quill::TriviallyCopyableTypeCodec`` helper.

.. code:: cpp

    #include "quill/bundled/fmt/ostream.h"
    #include "quill/bundled/fmt/format.h"

    #include "quill/TriviallyCopyableCodec.h"

    struct Order
    {
      char symbol[32];
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
    struct quill::Codec<Order> : quill::TriviallyCopyableTypeCodec<Order>
    {

    };

    Order order;
    strcpy(order.symbol, "AAPL");
    order.quantity = 100;
    order.price = 220.10;

    LOG_INFO(logger, "Order is {}", order);
    LOGV_INFO(logger, "Order", order);

Outputs:

    Order is symbol=AAPL price=220.1 quantity=100

    Order [order: symbol=AAPL price=220.1 quantity=100]

Serialising Trivially Copyable Types With Non-Default Constructor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For trivially copyable types with a non-default constructor, make ``quill::TriviallyCopyableTypeCodec<T>`` a friend and ensure there is a private default constructor.

.. code:: cpp

    #include "quill/bundled/fmt/ostream.h"
    #include "quill/bundled/fmt/format.h"

    #include "quill/TriviallyCopyableCodec.h"

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

      template <typename T>
      friend struct quill::TriviallyCopyableTypeCodec;

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
    struct quill::Codec<Order> : quill::TriviallyCopyableTypeCodec<Order>
    {
    };

    Order order {220.10, 100};

    LOG_INFO(logger, "Order is {}", order);
    LOGV_INFO(logger, "Order", order);

Outputs:

    Order is timestamp=17220422717461192 price=220.1 quantity=100

    Order [order: timestamp=17220422717461192 price=220.1 quantity=100]

Serialising Non Trivially Copyable User Defined Types With Public Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For user-defined types with non-trivially copyable types as members, it is necessary to define the class ``quill::Codec<T>``.

Note that it is possible to pass STL types to ``compute_total_encoded_size``, ``encode_members``, and ``decode_members`` as long as the relevant header file from ``quill/std/`` for that type is included.

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

Serialising User-Defined Types within STL Containers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
It is possible to log user-defined types nested within STL containers, such as ``std::vector<Order>``. To achieve this, ensure the following:

  1. Define a ``quill::Codec<T>`` specialization for the user-defined type as described above.
  2. Include the relevant header file for the STL type from the ``quill/std/`` directory.