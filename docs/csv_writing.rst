.. title:: CSV Writing

CSV Writing
===========

Use this page to write structured CSV data asynchronously using Quill's backend thread.

The library provides functionality for asynchronously writing CSV files. Formatting and I/O operations are managed by the backend thread, allowing for efficient and minimal-overhead CSV file writing on the hot path. This feature can be used alongside regular logging.

The :cpp:class:`CsvWriter` class is a utility designed to facilitate asynchronous CSV file writing.

Call :cpp:func:`CsvWriter::close` before stopping the backend worker if you need deterministic
logger removal and file closure. The destructor performs best-effort asynchronous cleanup and
does not block.

CSV Writing To File
-------------------

.. literalinclude:: ../examples/csv_writing.cpp
   :language: cpp
   :linenos:

Csv output (orders.csv):

.. code-block:: shell

    order_id,symbol,quantity,price,side
    13212123,AAPL,100,210.32,BUY
    132121123,META,300,478.32,SELL
    13212123,AAPL,120,210.42,BUY

Field Escaping
--------------

Fields are written verbatim. A string field containing a comma, double quote or line break
corrupts the CSV structure. For such fields, pass the value through
``quill::utility::csv_escape_field()`` from ``quill/Utility.h``, which quotes the field according
to RFC 4180 (fields without special characters are returned unchanged):

.. code-block:: cpp

    csv_writer.append_row(13212123, quill::utility::csv_escape_field("A,B \"C\""), 100, 210.32, "BUY");

.. note:: ``csv_escape_field()`` returns a new ``std::string``. When writing on a latency-sensitive
   path, prefer calling it only for fields that can actually contain special characters.

CSV Writing To Existing Sink
----------------------------
It is possible to pass an existing ``Sink``, or a custom user-created ``Sink``, to the CSV file for output. The following example shows how to use the console sink.

.. literalinclude:: snippets/quill_docs_example_csv_writer.cpp
   :language: cpp
   :linenos:
