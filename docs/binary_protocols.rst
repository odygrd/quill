.. title:: Binary Protocols

Binary Protocols
================

Logging Binary Protocols with Deferred Formatting
-------------------------------------------------
The library provides efficient logging of binary data in human-readable text format. While the logged data might be in
binary format initially, The library always produces text-based log files.
The ``BinaryDataDeferredFormatCodec`` enables efficient logging of variable-sized binary data by:

1. Copying the raw binary bytes on the hot path (critical performance section)
2. Deferring the expensive formatting operation to the backend logging thread

This approach is particularly useful for high-performance applications that need to log binary protocol messages like
custom binary formats without impacting application performance.

Implementation Steps
--------------------
To log binary data logging with deferred formatting, follow these steps:

1. Create a Tag Struct
~~~~~~~~~~~~~~~~~~~~~~
First, define an empty struct to serve as a tag for your binary protocol

.. code-block:: cpp

    struct MyBinaryProtocol { };

2. Define a Type Alias Using BinaryData
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Create a type alias using ``quill::BinaryData<T>`` to reference your binary data:

.. code-block:: cpp

    using MyBinaryProtocolData = quill::BinaryData<MyBinaryProtocol>;

3. Implement a Formatter for Your Binary Data Type
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Specialize the ``fmtquill::formatter`` template for your binary data type to define how it should be formatted in log messages:

.. code-block:: cpp

    template <>
    struct fmtquill::formatter<MyBinaryProtocolData>
    {
        constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

        auto format(::MyBinaryProtocolData const& bin_data, format_context& ctx) const
        {
            // Option 1: Convert binary data to hex representation
            return fmtquill::format_to(ctx.out(), "{}",
                quill::utility::to_hex(bin_data.data(), bin_data.size()));

            // Option 2: Parse binary data into a structured format
            // Custom parsing logic based on your protocol specification
            // return fmtquill::format_to(ctx.out(), "Field1: {}, Field2: {}", ...);
        }
    };

4. Specialize the Codec
~~~~~~~~~~~~~~~~~~~~~~~
Specialize the ``quill::Codec`` template to use ``BinaryDataDeferredFormatCodec`` for your binary data type:

.. code-block:: cpp

    template <>
    struct quill::Codec<MyBinaryProtocolData> : quill::BinaryDataDeferredFormatCodec<MyBinaryProtocolData>
    {
    };

5. Use in Your Logging Code
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Now you can log binary data efficiently:

.. code-block:: cpp

    // Assuming you have binary data in a buffer
    std::span<uint8_t> binary_buffer = get_binary_data();

    // Log the binary data - only a memcpy happens here (on the hot path)
    // The actual formatting will be deferred to the backend thread
    LOG_INFO(logger, "Received message: {}",
        MyBinaryProtocolData{binary_buffer.data(), binary_buffer.size()});

Using with SBE (Simple Binary Encoding)
---------------------------------------
SBE is a binary encoding protocol often used in financial systems. (See https://github.com/aeron-io/simple-binary-encoding for more information)

For SBE messages, you can leverage SBE's generated code to decode and format messages:

.. literalinclude:: ../examples/sbe_binary_data/sbe_logging.cpp
   :language: cpp
   :linenos:

Example Output
~~~~~~~~~~~~~~
The above example provides human-readable interpretation of the binary SBE messages while maintaining high performance
on the critical path. The log output might look like this.

.. code-block:: text

    [12:54:22.305465648] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 0, "price": 10000, "quantity": 1000, "side": "BUY", "symbol": "AAPL"}
    [12:54:22.305734067] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 1, "price": 11000, "quantity": 1500, "side": "SELL", "symbol": "MSFT"}
    [12:54:22.305734501] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 2, "price": 12000, "quantity": 2000, "side": "BUY", "symbol": "AMZN"}
    [12:54:22.305734827] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 3, "price": 13000, "quantity": 2500, "side": "SELL", "symbol": "GOOGL"}
    [12:54:22.305735141] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 4, "price": 14000, "quantity": 3000, "side": "BUY", "symbol": "META"}
    [12:54:22.305735433] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 5, "price": 15000, "quantity": 3500, "side": "SELL", "symbol": "TSLA"}
    [12:54:22.305735734] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 6, "price": 16000, "quantity": 4000, "side": "BUY", "symbol": "NVDA"}
    [12:54:22.305736031] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 7, "price": 17000, "quantity": 4500, "side": "SELL", "symbol": "PYPL"}
    [12:54:22.305736310] [SEND] {"Name": "NewOrder", "sbeTemplateId": 1, "orderId": 8, "price": 18000, "quantity": 5000, "side": "BUY", "symbol": "NFLX"}
    [12:54:22.305737653] [SEND] {"Name": "CancelOrder", "sbeTemplateId": 2, "orderId": 2000000, "origOrderId": 0, "cancelQuantity": 500, "reason": "User requested"}
    [12:54:22.305738178] [SEND] {"Name": "CancelOrder", "sbeTemplateId": 2, "orderId": 2000001, "origOrderId": 1, "cancelQuantity": 600, "reason": "Risk limit exceeded"}
    [12:54:22.305738429] [SEND] {"Name": "CancelOrder", "sbeTemplateId": 2, "orderId": 2000002, "origOrderId": 2, "cancelQuantity": 700, "reason": "Price away from market"}
    [12:54:22.305738723] [SEND] {"Name": "CancelOrder", "sbeTemplateId": 2, "orderId": 2000003, "origOrderId": 3, "cancelQuantity": 800, "reason": "Timeout"}
    [12:54:22.305738978] [SEND] {"Name": "CancelOrder", "sbeTemplateId": 2, "orderId": 2000004, "origOrderId": 4, "cancelQuantity": 900, "reason": "User requested"}

Binary Protocol Logging with Custom Message Types
-------------------------------------------------

In addition to SBE, you can use this approach with any binary protocol. This example demonstrates
logging different types of binary messages with custom formatting:

.. literalinclude:: ../examples/binary_protocol_logging.cpp
   :language: cpp
   :linenos:

Raw Binary File Writing
-----------------------

While Quill is primarily designed for human-readable text logging, it can also be configured to write raw bytes directly to files. This is useful for asynchronously writing binary data received from sources like network sockets, sensors, or other external systems to .bin files without any processing or formatting.

.. note::
   This is **not binary logging** - it's simply asynchronous writing of raw bytes to files using Quill's backend thread.

To write raw bytes to files, you **must** configure these critical options:

1. **Backend Options**: Disable character validation (global setting)
2. **Pattern Formatter**: Use message-only pattern with no suffix (for the binary logger only)

This example demonstrates asynchronously writing raw binary data to a file:

.. literalinclude:: examples/quill_docs_binary_file_writer.cpp
   :language: cpp
   :linenos:
