.. title:: Binary Protocols

Binary Protocols
================

Use this page to log binary protocol data (e.g., SBE messages) with deferred formatting on the backend thread, or to
write raw bytes to a file with ``LOG_BINARY``, for example a pcap capture.

Logging Binary Protocols with Deferred Formatting
-------------------------------------------------
The library provides efficient logging of binary data in human-readable text format. While the logged data might be in
binary format initially, these log statements always produce text-based log files. To write the bytes themselves, see
`Raw Binary File Writing`_.
The ``BinaryDataDeferredFormatCodec`` enables efficient logging of variable-sized binary data by:

1. Copying the raw binary bytes on the hot path (critical performance section)
2. Deferring the expensive formatting operation to the backend logging thread

This approach is particularly useful for high-performance applications that need to log binary protocol messages like
custom binary formats without impacting application performance.

Implementation Steps
--------------------
To log binary data with deferred formatting, follow these steps:

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

``LOG_BINARY(logger, data, size)`` writes bytes to the sinks of a logger as they are. This is useful for asynchronously
writing binary data received from sources like network sockets, sensors or market data feeds to a file, for example to
record a feed and replay it later.

The calling thread copies the bytes to its queue, as it does for the arguments of a log statement. The backend thread
orders the record by timestamp with the other events and passes the bytes to each sink without formatting them: no
pattern, newline or character sanitizing is applied, and the log levels and filters of the logger and its sinks are not
checked.

- ``FileSink`` and ``RotatingFileSink`` write the bytes to the file and add nothing, so the layout of the file is the
  layout of the records you log. A rotating sink does not split a record between two files.
- ``FileSinkConfig::set_binary_record_framing()`` makes the sink write the size of each record, or its timestamp and
  size, before its bytes, for records of different sizes. See `Framing Records`_.
- ``FileSinkConfig::set_file_header()`` sets bytes that start every file of the sink, including each file that a
  ``RotatingFileSink`` rotates to, e.g. a magic number, the global header of a pcap file or the header row of a csv
  file.
- A custom sink receives the record in ``write_binary(BinaryRecord const&)`` with its timestamp, thread, logger and the
  metadata of the ``LOG_BINARY`` call, such as its source location, and can frame or forward the bytes in any way. Sinks
  that do not implement it ignore binary records, and so do the JSON sinks and ``ConsoleSink``: a logger can have them
  for its log statements next to a file sink for its binary records. Ignored binary payloads contribute no bytes to
  ``RotatingJsonFileSink`` size rotation. A ``StreamSink`` writes the bytes to its stream.

A packet capture that ``tcpdump`` or Wireshark can read is written this way, see `Writing pcap Files`_.

.. note::
   Give the binary records a logger and a sink of their own: log statements of the same logger are written to the same
   file. A full dropping queue drops binary records as it drops log statements, so use a blocking queue, which is the
   default, for a recording that must be complete.

This example writes fixed-size records to a file and reads them back:

.. literalinclude:: snippets/quill_docs_binary_file_writer.cpp
   :language: cpp
   :linenos:

BinaryWriter
~~~~~~~~~~~~

:cpp:class:`BinaryWriterImpl`, as ``quill::BinaryWriter``, creates the file sink and a logger of its own for a file,
as :cpp:class:`CsvWriter` does for a csv file, so that a record is written with one call:

.. code-block:: cpp

    quill::FileSinkConfig sink_config;
    sink_config.set_open_mode('w');
    sink_config.set_file_header(file_header);

    quill::BinaryWriter writer{"orders.bin", sink_config};
    writer.write(&event, sizeof(event));

It also takes a ``RotatingFileSinkConfig``, or one or several sinks of your own. Use ``LOG_BINARY`` with a logger that
you create when the sinks should get the source location of each call.

Writers that are given the same file name share its sink, as csv writers do. A writer for a file whose sink writes
another file header or frames the records differently throws ``QuillError``.

The logger of a writer has the log level ``None``, so a log statement through it, e.g. from code that logs to every
logger of ``Frontend::get_all_loggers()``, is dropped instead of being written between the records.

Framing Records
~~~~~~~~~~~~~~~

Records of one size need nothing between them. For records of different sizes, the file sink can write a frame before
each record that tells a reader where it ends, in the byte order of the machine:

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

The timestamp is the one that the record already has in the queue, so the logging thread does nothing more for it. The
size of the frame counts towards ``rotation_max_file_size``, and a rotation does not separate a frame from its record.

``FileEventNotifier::before_write``, when it is set, transforms the bytes of each binary record as it does a log
statement and the header of the file, and the frame of a record has the size of the transformed bytes.

A header that you build yourself, e.g. with the time that the data was received, is written with the two-part
``BinaryWriter::write(header, header_size, data, size)`` or ``Logger::log_binary()``: the header and the data are
copied to the queue as one record, so the data is copied once and nothing comes between the two in the file.

``Logger::log_binary()`` takes the metadata that ``LOG_BINARY`` defines for its call site. The queue keeps the pointer,
so the metadata and its strings have to stay valid until the backend has written the record:

.. code-block:: cpp

    static constexpr quill::MacroMetadata metadata{
      "feed_handler.cpp:42", "on_packet", "", nullptr, quill::LogLevel::None, quill::MacroMetadata::Event::Binary};

    logger->log_binary(&metadata, header, header_size, data, size);

Writing pcap Files
------------------

:cpp:class:`PcapWriterImpl`, as ``quill::PcapWriter``, writes network packets to pcap files that ``tcpdump``,
Wireshark and other tools read. A pcap file is a file header of 24 bytes followed by the packets, each after a record
header of 16 bytes with its timestamp and size. The writer uses what is described above for both:

- The pcap file header is the file header of the sink, so every file starts with it, including each file that a rotation
  creates.
- ``write_packet(packet, size, timestamp_ns)`` builds the record header on the calling thread and copies it to the queue
  with the packet as one record. The packet is copied once, and a rotation does not come between the two.

A capture that starts a new file every hour:

.. code-block:: cpp

    quill::RotatingFileSinkConfig sink_config;
    sink_config.set_rotation_frequency_and_interval('H', 1);

    quill::PcapWriter pcap_writer{"quill_capture.pcap", sink_config};

    // An Ethernet frame that the application received, and the time it was received at
    pcap_writer.write_packet(frame, frame_size, receive_time_ns);

    // Writes the packets that are still queued and closes the file
    pcap_writer.close();

The complete program, which builds UDP packets to write, is
`examples/pcap_writing.cpp <https://github.com/odygrd/quill/blob/master/examples/pcap_writing.cpp>`_.
Read its capture with ``tcpdump``, or open it in Wireshark:

.. code-block:: shell

    $ tcpdump -nn -r quill_capture.pcap
    reading from file quill_capture.pcap, link-type EN10MB (Ethernet), snapshot length 262144
    02:44:29.852168 IP 10.0.0.1.5000 > 239.1.1.1.6000: UDP, length 21
    02:44:29.852219 IP 10.0.0.1.5000 > 239.1.1.1.6000: UDP, length 21
    02:44:29.852220 IP 10.0.0.1.5000 > 239.1.1.1.6000: UDP, length 21

The link type of the packets and the snapshot length, the most bytes that are kept of a packet, are arguments of the
constructor. The timestamps in the file are in nanoseconds.

- By default the file sink appends. The writer then requires a file that is not empty to be a capture with the same
  byte order, timestamp precision, snapshot length and link type, and throws ``QuillError`` otherwise: the packets it
  adds are read with the header that is in the file. The sink makes the same check of every file it opens later, e.g.
  after the file was removed, and stays closed for a file that fails it.
- Writers that are given the same file name share its sink, as csv and binary writers do. A writer for a file whose
  sink does not write the same pcap file header, e.g. the sink of a writer with another link type, throws
  ``QuillError``.
- ``write_packet(packet, captured_size, original_size, timestamp_ns)`` is for a source that has kept only the first
  bytes of a packet. The file has both sizes.
- A timestamp or a size that the fields of a pcap record header cannot hold throws ``QuillError``.
