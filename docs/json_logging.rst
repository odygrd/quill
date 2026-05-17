.. title:: JSON Logging

JSON Logging
============

Use this page to output structured JSON logs to files or the console, optionally alongside standard pattern-formatted output.

The library supports outputting structured JSON logs to either the console or a file. To utilize this feature, use named placeholders in the format string (e.g., ``{name}`` instead of ``{}``) when invoking the ``LOG_`` macros.

For convenience, the ``LOGJ_`` macros offer an alternative method for logging l-values, automatically including the argument names in the placeholders. These macros support up to 26 arguments.

For details on how named arguments work in Quill, including important differences from ``fmtlib``'s ``fmt::arg()``, see :ref:`Named Placeholders <logging_macros:Named Placeholders>` in the Logging Macros guide.

It is also possible to combine JSON output with standard log pattern display by passing multiple ``Sink`` objects to a ``Logger``. This allows you to see the same log message in different formats simultaneously.

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

To customize the JSON format, define a custom sink that derives from one of the following classes:

- :cpp:class:`JsonFileSink`
- :cpp:class:`JsonConsoleSink`
- :cpp:type:`RotatingJsonFileSink`

.. literalinclude:: ../examples/json_console_logging_custom_json.cpp
   :language: cpp
   :linenos:

Combining JSON and Standard Log Patterns
----------------------------------------

.. literalinclude:: ../examples/json_file_logging.cpp
   :language: cpp
   :linenos:
