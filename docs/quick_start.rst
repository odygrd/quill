.. title:: Quick Start

Quick Start
===========

Use this page for the smallest working setup, then move to the full API when you
need more control over sinks, loggers, queues, or lifecycle.

Quickest Setup
--------------

For the quickest and simplest setup, use ``simple_logger()`` with the recommended ``LOG_*`` macros:

.. literalinclude:: snippets/quill_docs_quick_start.cpp
   :language: cpp
   :linenos:

Alternatively, a **macro-free interface** is available if you prefer function-call syntax:

.. code-block:: cpp

   #include "quill/LogFunctions.h"
   #include "quill/SimpleSetup.h"

   int main()
   {
     auto* logger = quill::simple_logger();
     quill::info(logger, "Hello from {}!", "Quill");
   }

The ``LOG_*`` macros are recommended for lowest latency — they avoid evaluating arguments when the
log level is disabled and resolve metadata at compile time. The macro-free functions
(``quill::info()``, ``quill::warning()``, etc.) offer cleaner syntax with slightly higher overhead.
See :doc:`Macro-Free Mode <macro_free_mode>` for the full trade-off comparison.

.. note::

   - **Use ``simple_logger()``** when you want the smallest setup with one or two default loggers.
   - **Use ``Backend`` / ``Frontend``** when you need custom sinks, multiple loggers, custom formatter patterns, backend options, frontend options, or explicit lifecycle control.
   - **See also:** a standalone version of this convenience path is available in ``examples/simple_setup.cpp``.

Architecture Overview
---------------------

The library is header only and consists of two main components: the frontend and the backend.

- **Frontend:** captures a copy of the log arguments and metadata from each ``LOG_*`` statement and places them in a thread-local SPSC (Single Producer Single Consumer) queue buffer. Each frontend thread has its own lock-free queue, ensuring no contention between logging threads.

- **Backend:** runs in a separate thread, spawned by the library, asynchronously consuming messages from all frontend queues, formatting them, and writing them to the configured sinks.

Detailed Setup
--------------

For more detailed control, start the backend thread in your application, then set up one or more output ``Sinks`` and a ``Logger``.

Once the initialization is complete, you only need to include two header files to issue log statements:

- ``#include "quill/LogMacros.h"``
- ``#include "quill/Logger.h"``

These headers have minimal dependencies, keeping compilation times low.

For larger projects, the recommended setup is to wrap Quill initialization and setup in your own
small static library that you build once and link into the rest of the application, as shown in:
`Recommended Usage Example <https://github.com/odygrd/quill/tree/master/examples/recommended_usage>`_.

For a quick reference on usage see :doc:`Recipes <recipes>`.

Logging to Console
~~~~~~~~~~~~~~~~~~

.. literalinclude:: snippets/quill_docs_example_console.cpp
   :language: cpp
   :linenos:

Expected output:

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

- :doc:`Overview <overview>` for the full architecture and design rationale.
- :doc:`Metrics <metrics>` for publishing compact metric samples through the same backend worker — including the built-in Prometheus exporter.
- :doc:`MDC <mdc>` for attaching per-thread request/session context to subsequent log lines.
- :doc:`JSON Logging <json_logging>` for structured machine-friendly output.
- :doc:`Backend Options <backend_options>` and :doc:`Frontend Options <frontend_options>` for tuning queues, clocks, and backend behavior.
- :doc:`Guides <guides>` for sinks, formatters, filters, backtrace logging, and more.
- :doc:`Recipes <recipes>` for common tasks and code examples, including STL and user-defined type logging.
