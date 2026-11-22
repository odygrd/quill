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

One Backend Thread for Every Output
-----------------------------------

One backend thread does the formatting and I/O for your logs, metrics, data files and packet
captures.

By default, every call below costs your thread one thing: encoding its arguments into a lock-free
queue of its own, which for most types is a plain copy. Formatting and I/O run later on the backend
thread, which merges everything in timestamp order across all threads. Where a record goes, to a
file, a terminal or a metrics collector, does not change what it costs your thread.

.. list-table::
   :header-rows: 1
   :widths: 34 36 30

   * - Use Quill to
     - Your thread calls
     - The backend thread writes
   * - Log :doc:`text <sink_types>` to the console, to files rotated by size or time, or to syslog, the systemd journal and Android logcat
     - ``LOG_INFO(logger, "User {} logged in from {}", user, address);``
     - ``User alice logged in from 10.0.0.5``, with the time, thread and source location of your
       pattern
   * - Write structured :doc:`JSON <json_logging>` logs, for a log pipeline to search
     - ``LOGJ_INFO(logger, "User logged in", user, address);``
     - ``{…,"user":"alice","address":"10.0.0.5"}``
   * - Log :ref:`your own types <recipes:Logging User Defined Types>` with the formatter they already have
     - ``LOG_INFO(logger, "Connected to {}", endpoint);``
     - ``Connected to example.org:443``
   * - Publish :doc:`metrics <metrics>` to any metrics system, with a built-in sink for Prometheus
     - ``METRIC(metrics_logger, requests_total, 1.0);``
     - Each sample to your metrics sink, e.g. ``requests_total 1024`` for Prometheus
   * - Write :doc:`CSV files <csv_writing>`
     - ``weather_csv.append_row("Athens", 21.4, 40);``
     - ``Athens,21.4,40``, below the header ``city,temperature,humidity`` of every file
   * - Record :ref:`binary data <binary_protocols:Raw Binary File Writing>` as it arrives, such as a sensor stream or a market data feed
     - ``LOG_BINARY(logger, &reading, sizeof(reading));``
     - The bytes as they are, or after their size and timestamp
   * - Capture :ref:`network traffic <binary_protocols:Writing pcap Files>`
     - ``pcap_writer.write_packet(frame, frame_size, timestamp_ns);``
     - A capture that Wireshark and tcpdump open, in a new file every hour if you like
   * - Decode :doc:`binary protocols <binary_protocols>` such as SBE into readable text
     - ``LOG_INFO(logger, "[IN] {}", SbeMessage{buffer, size});``
     - The fields of each message, decoded on the backend thread
   * - Send records :doc:`anywhere <sinks>`: a database, a socket, a message bus
     - A sink of your own, with ``write_log(LogRecord const&)``
     - Whatever your sink does with each record

An application can have as many loggers and writers as it needs, for any mix of the above. They all
share the one backend thread, and a call still costs its thread only the encoding into its queue:

- **A trading system** logs its decisions as text, records the exchange feed to pcap files that
  rotate every hour, writes its fills to CSV, and publishes its order latencies as metrics.
- **A multiplayer game server** keeps I/O out of its tick loop: it logs to rotating files, records
  each match as a binary replay, and publishes its tick time and the number of players online as
  metrics.
- **A VoIP server** logs to syslog, captures the packets of its calls to pcap files for Wireshark,
  and publishes the number of active calls as metrics.
- **A service in a container** logs JSON to stdout for the platform to collect, and publishes its
  request counts as metrics.

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
