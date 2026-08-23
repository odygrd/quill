:hide-navigation:

.. meta::
   :google-site-verification: OdGHhtE4NLXZfqvQWdVxnV4z8MJeRUws09jAYCDjqhI
   :description: Quill is an ultra-low-latency asynchronous C++17 logging and metrics library with deferred formatting, structured logging, and extensible sinks.

.. _index:

Quill: Ultra-Low-Latency C++ Logging and Metrics
=================================================

.. rst-class:: quill-home-lead

Quill is a C++17 library for applications that need to keep log formatting and sink I/O away
from latency-sensitive threads.

An enabled ``LOG_*`` call places a pointer to static call-site metadata and encoded copies of its
arguments into the calling thread's SPSC queue. A dedicated backend worker decodes and formats
events, compares available events by timestamp, and dispatches them to sinks.

.. container:: quill-home-links

   :doc:`Get started <quick_start>` · :doc:`Install Quill <installing>` ·
   `Review the benchmarks <https://github.com/odygrd/quill#user-content--performance>`_

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

A macro-free interface (``quill::info()``, ``quill::warning()``, ...) is also available. The
recommended ``LOG_*`` macros avoid evaluating arguments when a log level is disabled and keep
call-site metadata in a static object. See :doc:`Macro-Free Mode <macro_free_mode>` for the
alternative interface and its performance trade-offs.

Use :doc:`Quick Start <quick_start>` for the smallest setup, or move to the full ``Backend`` and
``Frontend`` APIs when you need custom sinks, multiple loggers, metrics, or explicit lifecycle
control.

.. raw:: html

   <div class="quill-feature-grid"><section>

Keep formatting off latency-sensitive threads
----------------------------------------------

``LOG_*`` binary-encodes copies of its arguments into the calling thread's queue. The backend
decodes and formats the event, then performs sink I/O. Published frontend-latency and throughput
results include the benchmark system and methodology rather than presenting one number as
universal.

:doc:`How Quill works <overview>` ·
`Benchmark methodology <https://github.com/odygrd/quill#user-content--performance>`_

.. raw:: html

   </section><section>

Publish logs and metrics through one backend
--------------------------------------------

Register metric names and labels once; the hot path carries a stable ``MetricMetadata`` pointer
and a ``double`` sample through the same backend used for logs. The built-in ``PrometheusSink``
supports counters, gauges, histograms, and summaries. Custom sinks can route samples elsewhere;
the core metric API transports samples, while aggregation and storage belong to the selected sink
or external metrics system.

:doc:`Publish metrics <metrics>`

.. raw:: html

   </section><section>

Emit structured data
--------------------

Named placeholders preserve field names for JSON output. Built-in sinks cover JSON console,
file, and rotating-file output, and one logger can send the same event to both JSON and standard
pattern-formatted sinks.

:doc:`Configure JSON logging <json_logging>`

.. raw:: html

   </section><section>

Extend types and destinations
-----------------------------

Optional headers under ``quill/std`` provide codecs for common standard-library types, including
containers, ``std::chrono``, ``std::variant``, and ``std::error_code``. User-defined types can
provide formatting support and a ``Codec`` specialization. Built-in destinations include console,
file, rotating file, Syslog, systemd, Android, JSON, and Prometheus; custom destinations derive
from ``Sink``.

:doc:`Type recipes <recipes>` · :doc:`Browse sink types <sink_types>`

.. raw:: html

   </section><section>

Add context and control
-----------------------

Mapped Diagnostic Context attaches thread-local key/value fields to subsequent records without
re-enqueueing the full context on every log call. Per-sink filters and rate-limited macros help
control noisy streams. Backtrace logging keeps selected low-level records in a backend ring buffer
for on-demand or high-severity-triggered flushing.

:doc:`Mapped Diagnostic Context <mdc>` · :doc:`Filters <filters>` ·
:doc:`Backtrace logging <backtrace_logging>`

.. raw:: html

   </section><section>

Configure the complete pipeline
-------------------------------

``FrontendOptions`` is a compile-time traits type for queue mode and capacity, blocking retries,
and Linux huge-page policy. At runtime, ``BackendOptions`` controls idle behaviour and CPU
affinity, transit-buffer limits, timestamp handling, flushing, shutdown draining, callbacks, and
character sanitization. Loggers select their format pattern, clock source, and one or more sinks;
sinks can be shared, filtered, or user-defined for logs, metrics, or both.

:doc:`Frontend options <frontend_options>` · :doc:`Backend options <backend_options>` ·
:doc:`Sinks and extensions <sinks>`

.. raw:: html

   </section></div>

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
