.. title:: Sink Types

Sink Types
==========

ConsoleSink
~~~~~~~~~~~

The `ConsoleSink` class sends logging output to streams `stdout` or `stderr`. Printing color codes to terminal or Windows console is also supported.

FileSink
~~~~~~~~

The :cpp:class:`FileSink` is a straightforward sink that outputs to a file. The filepath of the `FileSink` serves as a unique identifier, allowing you to retrieve the same sink later using :cpp:func:`FrontendImpl::get_sink`.

Each file can only have a single instance of `FileSink`.

.. literalinclude:: examples/quill_docs_example_file.cpp
   :language: cpp
   :linenos:

RotatingFileSink
~~~~~~~~~~~~~~~~

The :cpp:class:`RotatingFileSink` is built on top of the `FileSink` and provides log file rotation based on specified time intervals, file sizes, or daily schedules.

.. literalinclude:: ../examples/rotating_file_logging.cpp
   :language: cpp
   :linenos:

JsonFileSink/JsonConsoleSink
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. literalinclude:: examples/quill_docs_example_json_logging.cpp
   :language: cpp
   :linenos:

RotatingJsonFileSink
~~~~~~~~~~~~~~~~~~~~

The :cpp:class:`RotatingJsonFileSink` is built on top of the `JsonFileSink` and provides log file rotation based on specified time intervals, file sizes, or daily schedules.

.. literalinclude:: ../examples/rotating_json_file_logging.cpp
   :language: cpp
   :linenos:
