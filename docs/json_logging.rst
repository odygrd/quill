.. title:: JSON Logging

JSON Logging
============

Send structured JSON to a file or the console. Attach both a JSON sink and a text sink to the same logger
to see each event in both formats.

Name fields with ``{name}`` in ``LOG_*``, or let ``LOGJ_*`` name up to 26 l-values automatically. See
:ref:`logging_macros:named placeholders` for positional matching and the difference from ``fmt::arg()``.

The default JSON sinks reserve ``timestamp``, ``file_name``, ``line``, ``thread_id``, ``logger``, ``log_level`` and
``message``. A colliding argument name gets ``_1``, ``_2`` and so on until unique.

Logging Json to Console
-----------------------

.. literalinclude:: ../examples/json_console_logging.cpp
   :language: cpp
   :linenos:

Logging Json to File
--------------------

.. literalinclude:: snippets/quill_docs_example_json_logging.cpp
   :language: cpp
   :linenos:

Customising Json Format
-----------------------

Derive a custom sink from :cpp:class:`JsonFileSink`, :cpp:class:`JsonConsoleSink` or
:cpp:type:`RotatingJsonFileSink` to change its output.

.. literalinclude:: ../examples/json_console_logging_custom_json.cpp
   :language: cpp
   :linenos:

Combining JSON and Standard Log Patterns
----------------------------------------

.. literalinclude:: ../examples/json_file_logging.cpp
   :language: cpp
   :linenos:
