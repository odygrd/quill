.. title:: Quick Start

.. meta::
   :description: Get started with Quill, an ultra-low-latency asynchronous C++ logging library, using a minimal C++ example with macros or function calls.

Quick Start
===========

Start with a console or file logger. Use the full API when you need custom sinks, patterns, queues, multiple
logger configurations or explicit control over startup and shutdown.

Quickest Setup
--------------

For one or two default loggers, call ``simple_logger()`` and use the recommended ``LOG_*`` macros.

.. literalinclude:: snippets/quill_docs_quick_start.cpp
   :language: cpp
   :linenos:

For function-call syntax, include ``quill/LogFunctions.h``.

.. literalinclude:: snippets/quill_docs_log_functions.cpp
   :language: cpp
   :linenos:

See ``examples/simple_setup.cpp`` for a standalone example.
See :doc:`macro_free_mode` for argument evaluation and performance differences.

Architecture Overview
---------------------

Quill is header-only. Application threads enqueue records and one worker writes them to sinks.
The :doc:`overview` shows the queue and record layout.

Detailed Setup
--------------

Start the backend, create one or more sinks, then create the logger. Translation units that log need only
``quill/LogMacros.h`` and ``quill/Logger.h``. For larger projects, compile initialization once in a small
static library, as in the `recommended usage example <https://github.com/odygrd/quill/tree/master/examples/recommended_usage>`_.

Logging to Console
~~~~~~~~~~~~~~~~~~

.. literalinclude:: snippets/quill_docs_example_console.cpp
   :language: cpp
   :linenos:

**Console output**

.. code-block:: text

   20:07:26.653631750 [28794] example_console.cpp:26   LOG_INFO      root         A log message with number 123
   20:07:26.653631950 [28794] example_console.cpp:30   LOG_INFO      root         libfmt formatting language is supported 3.14e+00
   20:07:26.653632050 [28794] example_console.cpp:34   LOG_INFO      root         Logging STD types is supported [1, 2, 3]
   20:07:26.653632150 [28794] example_console.cpp:37   LOG_INFO      root         Logging STD types is supported [arr: [1, 2, 3]]
   20:07:26.653632250 [28794] example_console.cpp:41   LOG_INFO      root         A message with two variables [a: 123, b: 3.17]

Logging to File
~~~~~~~~~~~~~~~

.. literalinclude:: snippets/quill_docs_example_file.cpp
   :language: cpp
   :linenos:

Next Steps
----------

.. rst-class:: quill-link-list

- :doc:`Metrics <metrics>` for samples, exporters and the built-in Prometheus sink.
- :doc:`MDC <mdc>` for request or session fields and :doc:`JSON logging <json_logging>` for structured output.
- :doc:`Backend options <backend_options>` and :doc:`frontend options <frontend_options>` for worker and queue settings.
- :doc:`Guides <guides>` for clocks, sinks, formatters, filters and backtraces.
- :doc:`Recipes <recipes>` for common tasks, including logging STL and user-defined types.
