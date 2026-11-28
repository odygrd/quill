.. title:: Binary Protocols

Binary Protocols
================

Decode a binary protocol such as SBE into readable logs, or record its original bytes for replay. Use
``LOG_BINARY`` for raw data and ``PcapWriter`` for captures that Wireshark and tcpdump can read.

Logging Binary Protocols with Deferred Formatting
-------------------------------------------------
``BinaryDataDeferredFormatCodec`` copies a variable-length payload on the caller and runs its formatter on
the backend. This produces text. To keep the bytes, see `Raw Binary File Writing`_.

Implementation Steps
--------------------
The following steps connect a protocol's buffer type, formatter and codec.

1. Create a Tag Struct
~~~~~~~~~~~~~~~~~~~~~~
Use an empty type to identify the protocol.

.. code-block:: cpp

    struct MyBinaryProtocol { };

2. Define a Type Alias Using BinaryData
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Name the corresponding ``quill::BinaryData<T>`` view.

.. code-block:: cpp

    using MyBinaryProtocolData = quill::BinaryData<MyBinaryProtocol>;

3. Implement a Formatter for Your Binary Data Type
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Specialize ``fmtquill::formatter`` for the view. Decode the fields you want in the output.

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
Use ``BinaryDataDeferredFormatCodec`` in the view's ``quill::Codec`` specialization.

.. code-block:: cpp

    template <>
    struct quill::Codec<MyBinaryProtocolData> : quill::BinaryDataDeferredFormatCodec<MyBinaryProtocolData>
    {
    };

5. Use in Your Logging Code
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Pass the buffer and its length to the log statement.

.. code-block:: cpp

    // Assuming you have binary data in a buffer
    std::span<uint8_t> binary_buffer = get_binary_data();

    // Log the binary data - only a memcpy happens here (on the hot path)
    // The actual formatting will be deferred to the backend thread
    LOG_INFO(logger, "Received message: {}",
        MyBinaryProtocolData{binary_buffer.data(), binary_buffer.size()});

Using with SBE (Simple Binary Encoding)
---------------------------------------
`Simple Binary Encoding (SBE) <https://github.com/aeron-io/simple-binary-encoding>`_ is used in financial
systems. The formatter below uses the generated message decoder.

.. literalinclude:: ../examples/sbe_binary_data/sbe_logging.cpp
   :language: cpp
   :linenos:

Example Output
~~~~~~~~~~~~~~
The decoded fields appear as text.

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

The same approach handles other protocols. This example formats several custom message types.

.. literalinclude:: ../examples/binary_protocol_logging.cpp
   :language: cpp
   :linenos:

Raw Binary File Writing
-----------------------

``LOG_BINARY(logger, data, size)`` copies ``size`` bytes into the caller's queue. The backend orders the record
by timestamp and delivers it without formatting, newline, sanitization, severity checks or sink filters.
Use it for network, sensor or market data feeds you want to replay.

- ``FileSink`` and ``RotatingFileSink`` write the bytes verbatim by default. Rotation keeps a record in one file.
  For variable lengths use ``FileSinkConfig::set_binary_record_framing()`` (see `Framing Records`_).
- ``FileSinkConfig::set_file_header()`` starts each new file, including rotations, with bytes such as a magic
  number, global pcap header or CSV header row. See `Writing pcap Files`_.
- A custom ``write_binary(BinaryRecord const&)`` can frame or forward the payload using its timestamp, thread,
  logger and call-site metadata. ``StreamSink`` writes bytes to its stream. The default callback, JSON sinks
  and ``ConsoleSink`` ignore them, so those sinks may coexist with a binary file sink. Ignored payloads do
  not count toward ``RotatingJsonFileSink`` size rotation.

Use a dedicated logger and file sink to keep text out of a binary recording. A full dropping queue loses
binary records just like logs. Choose a blocking queue (the default) when the recording must be complete.

The example writes fixed-size records and reads them back.

.. literalinclude:: snippets/quill_docs_binary_file_writer.cpp
   :language: cpp
   :linenos:

BinaryWriter
~~~~~~~~~~~~

``quill::BinaryWriter`` (:cpp:class:`BinaryWriterImpl`) creates a file sink and private logger, as
:cpp:class:`CsvWriter` does for CSV. Write a record with one call.

.. code-block:: cpp

    quill::FileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_file_header(file_header);

    quill::BinaryWriter writer{"orders.bin", sink_config};
    writer.write(&event, sizeof(event));

The writer also accepts ``RotatingFileSinkConfig`` or one or more supplied sinks. Use ``LOG_BINARY`` on your
own logger if a sink needs the source location of each log site.

Writers for the same filename share its sink. Incompatible file headers or record framing throw ``QuillError``.
The writer logger's level is ``None``, so regular messages, including those sent via ``Frontend::get_all_loggers()``,
cannot interleave text with its data.

Framing Records
~~~~~~~~~~~~~~~

Fixed-size records need no separator. For variable sizes, choose a prefix in machine byte order.

.. list-table::
   :header-rows: 1

   * - ``BinaryRecordFraming``
     - Written before the bytes of each record
   * - ``None``
     - Nothing. This is the default.
   * - ``Size``
     - The number of bytes of the record, as a ``uint32_t``
   * - ``TimestampAndSize``
     - The timestamp of the record in nanoseconds since epoch, as a ``uint64_t``, then the number of bytes of the
       record, as a ``uint32_t``

.. code-block:: cpp

    quill::FileSinkConfig sink_config;
    sink_config.set_binary_record_framing(quill::BinaryRecordFraming::TimestampAndSize);

    quill::BinaryWriter writer{"feed.bin", sink_config};
    writer.write(message, message_size);

The frame reuses the queued timestamp and counts toward ``rotation_max_file_size``. A frame and its payload
always rotate together. ``FileEventNotifier::before_write`` can transform records and file headers. The
size in the frame describes the transformed payload.

For a caller-built header (e.g. receive time), ``BinaryWriter::write(header, header_size, data, size)``
or ``Logger::log_binary()`` copies both parts as one record. The payload is copied once and remains
adjacent to its header.

``Logger::log_binary()`` retains a pointer to call-site metadata normally supplied by ``LOG_BINARY``.
Keep the metadata and its strings valid until the backend writes the record.

.. code-block:: cpp

    static constexpr quill::MacroMetadata metadata{
      "feed_handler.cpp:42", "on_packet", "", nullptr, quill::LogLevel::None, quill::MacroMetadata::Event::Binary};

    logger->log_binary(&metadata, header, header_size, data, size);

Writing pcap Files
------------------

``quill::PcapWriter`` (:cpp:class:`PcapWriterImpl`) starts each file, including rotations, with the 24-byte
pcap header. ``write_packet(packet, size, timestamp_ns)`` builds a 16-byte timestamp and size header on the
caller and enqueues it with the packet. The packet is copied once and rotates with its header.

This capture starts a new file every hour.

.. code-block:: cpp

    quill::RotatingFileSinkConfig sink_config;
    sink_config.set_rotation_frequency_and_interval('H', 1);

    quill::PcapWriter pcap_writer{"quill_capture.pcap", sink_config};

    // An Ethernet frame that the application received, and the time it was received at
    pcap_writer.write_packet(frame, frame_size, receive_time_ns);

    // Writes the packets that are still queued and closes the file
    pcap_writer.close();

`examples/pcap_writing.cpp <https://github.com/odygrd/quill/blob/master/examples/pcap_writing.cpp>`_ builds
the UDP packets. Open the capture in Wireshark or read it with ``tcpdump``.

.. code-block:: shell

    $ tcpdump -nn -r quill_capture.pcap
    reading from file quill_capture.pcap, link-type EN10MB (Ethernet), snapshot length 262144
    02:44:29.852168 IP 10.0.0.1.5000 > 239.1.1.1.6000: UDP, length 21
    02:44:29.852219 IP 10.0.0.1.5000 > 239.1.1.1.6000: UDP, length 21
    02:44:29.852220 IP 10.0.0.1.5000 > 239.1.1.1.6000: UDP, length 21

Set link type and snapshot length (maximum captured bytes per packet) in the constructor. Timestamps
use nanoseconds.

- The default file mode appends. For a non-empty file, byte order, timestamp precision, snapshot length and
  link type must match its existing header or construction throws ``QuillError``. The sink rechecks on every
  later open, including after file removal, and stays closed on mismatch.
- Writers sharing a filename must agree on its pcap header, including link type, or creation throws ``QuillError``.
- For truncated input, ``write_packet(packet, captured_size, original_size, timestamp_ns)`` records both sizes.
- A timestamp or size outside the pcap record field range throws ``QuillError``.
