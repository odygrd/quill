.. title:: CSV Writing

CSV Writing
===========

Use :cpp:class:`CsvWriter` to write rows alongside ordinary logs. The caller submits fields and the backend
formats and writes them.

**Files and headers.** The default mode appends, preserving existing rows. A writer created by filename
uses ``FileSinkConfig::set_file_header()`` so each new or rotated file gets one header and a non-empty file
gets no duplicate. Pass ``'w'`` or a ``FileSinkConfig`` with ``set_open_mode('w')`` to truncate.

**Sharing.** Writers for the same filename share the sink. With the same schema they can be created in any
order using default header settings. The first writer sets the sink's header. Requesting a different or
previously disabled header throws ``QuillError``. Writers given a sink emit their own header row, so pass
``should_write_header=false`` to all but one, or share a single writer.

**Closing.** Finish submitting rows before calling :cpp:func:`CsvWriter::close`. Call it while the backend
runs for deterministic logger removal. The destructor requests best-effort asynchronous cleanup without
blocking. A shared file closes when its last sink owner releases it.

CSV Writing To File
-------------------

.. literalinclude:: ../examples/csv_writing.cpp
   :language: cpp
   :linenos:

``orders.csv`` after the first run. Later runs append rows.

.. code-block:: shell

    order_id,symbol,quantity,price,side
    13212123,AAPL,100,210.32,BUY
    132121123,META,300,478.32,SELL
    13212123,AAPL,120,210.42,BUY

Field Escaping
--------------

CSV fields are written verbatim. For strings that can contain commas, double quotes or line breaks, use
``quill::utility::csv_escape_field()`` from ``quill/Utility.h``. It applies RFC 4180 quoting and leaves the
contents of ordinary fields unchanged.

.. code-block:: cpp

    csv_writer.append_row(13212123, quill::utility::csv_escape_field("A,B \"C\""), 100, 210.32, "BUY");

.. note:: The helper returns a new ``std::string``. On a hot path call it only for fields that can need quoting.

CSV Writing To Existing Sink
----------------------------
Pass an existing or custom sink to choose the destination. This example writes to the console.

.. literalinclude:: snippets/quill_docs_example_csv_writer.cpp
   :language: cpp
   :linenos:
