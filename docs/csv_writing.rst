.. title:: CSV Writing

CSV Writing
===========

The library provides functionality for asynchronously writing CSV files. Formatting and I/O operations are managed by the backend thread, allowing for efficient and minimal-overhead CSV file writing on the hot path. This feature can be used alongside regular logging.

The :cpp:class:`CsvWriter` class is a utility designed to facilitate asynchronous CSV file writing.

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

CSV Writing To Existing Sink
----------------------------
It is possible to pass an existing `Sink`, or a custom user-created `Sink`, to the CSV file for output. The following example shows how to use the console sink

.. literalinclude:: examples/quill_docs_example_csv_writer_1.cpp
   :language: cpp
   :linenos:
