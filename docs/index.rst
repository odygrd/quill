.. meta::
   :google-site-verification: OdGHhtE4NLXZfqvQWdVxnV4z8MJeRUws09jAYCDjqhI

.. _index:

Fast asynchronous logging and metrics for low-latency C++ applications.

Quill keeps formatting and I/O off your hot threads: ``LOG_*`` macros binary-serialize arguments into a thread-local lock-free queue, and a single backend worker reconstructs, timestamp-orders, formats, and writes them.

Quick Example
-------------

.. code-block:: cpp

   #include "quill/LogMacros.h"
   #include "quill/SimpleSetup.h"

   int main()
   {
     auto* logger = quill::simple_logger();
     LOG_INFO(logger, "Hello from {}!", "Quill");
   }

A macro-free interface (``quill::info()``, ``quill::warning()``, ...) is also available — see :doc:`Macro-Free Mode <macro_free_mode>`.

Use :doc:`Quick Start <quick_start>` for the smallest setup, or move to the full
``Backend`` / ``Frontend`` APIs when you need custom sinks, multiple loggers, metrics,
or more explicit lifecycle control.

Highlights
----------

- **Nanosecond hot path** — binary-copy arguments into a per-thread SPSC queue, defer formatting to the backend.
- **Timestamp-ordered logs** — a single backend worker merges events from all frontend queues chronologically.
- **Publish metrics on the same pipeline** — send :doc:`pre-registered metric samples <metrics>` to Prometheus, StatsD, OpenTelemetry, or any custom collector through the same backend worker (built-in ``PrometheusSink`` included). Quill is the low-latency transport, not a metrics system itself.
- **Mapped Diagnostic Context** — attach per-thread key/value context that appears on subsequent log lines automatically. See :doc:`MDC <mdc>`.
- **Rich STL and user-type support** — codecs for ``std::vector``, ``std::map``, ``std::variant``, ``std::chrono``, ``std::bitset``, ``std::complex``, ``std::error_code``, nested containers, and your own types.
- **Structured JSON output**, rotating sinks, filters, backtrace logging, custom clocks, and more.

Start Here
----------

- :doc:`Get Started <quick_start>` for the shortest path to working logs
- :doc:`Installing <installing>` for package manager and source setup
- :doc:`Guides <guides>` for sinks, metrics, formatters, JSON, filters, and more
- :doc:`Recipes <recipes>` for common tasks and examples
- :doc:`FAQ <faq>` for integration guidance and common pitfalls

.. toctree::
   :maxdepth: 2
   :caption: Home
   :hidden:

   self

.. toctree::
   :maxdepth: 2
   :caption: Get Started
   :hidden:

   quick_start
   installing
   basic_example
   overview
   faq

.. toctree::
   :maxdepth: 2
   :caption: Guides
   :hidden:

   guides

.. toctree::
   :maxdepth: 2
   :caption: Recipes
   :hidden:

   recipes

.. toctree::
   :maxdepth: 2
   :caption: API
   :hidden:

   users-api
