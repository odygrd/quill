:hide-navigation:

.. meta::
   :google-site-verification: OdGHhtE4NLXZfqvQWdVxnV4z8MJeRUws09jAYCDjqhI
   :description: Quill is an ultra-low-latency asynchronous C++17 logging and metrics library with deferred formatting, structured logging, and extensible sinks.

.. _index:

**Quill**: Ultra-low-latency C++ logging and metrics
====================================================

.. container:: quill-home-intro

   Quill keeps formatting and I/O off your application threads. In **C++17 and later**,
   a log call captures its arguments for a dedicated worker to process asynchronously.

   .. rst-class:: quill-home-links

   - :doc:`Get started <quick_start>`
   - :doc:`View benchmarks <benchmarks>`

.. container:: quill-example

   .. rubric:: :si-icon:`octicons/code-16` Quick Example
      :heading-level: 2
      :name: quick-example

   .. container:: quill-example-preview

      .. literalinclude:: snippets/quill_docs_home.cpp
         :language: cpp

      .. container:: quill-example-output

         :si-icon:`octicons/terminal-16` **Log message** ``Listening on ports [8080, 8443]``

   .. raw:: html

      <div class="quill-example-actions">
        <button class="quill-example-run" type="button" hidden>Run this example in Compiler Explorer</button>
        <a class="quill-example-open" href="{{ compiler_explorer_url }}" target="_blank" rel="noopener noreferrer">Open in Compiler Explorer <span aria-hidden="true">↗</span></a>
      </div>

   .. rst-class:: quill-example-version

   Compiler Explorer runs GCC 14.2 with Quill 13.1.0 until Quill 14 is available there.

Asynchronous logging features
-----------------------------

.. container:: quill-card-grid

   .. container:: quill-card
      :name: low-latency-on-the-calling-thread

      .. rubric:: :si-icon:`octicons/zap-16` :doc:`Low frontend latency <benchmarks_latency>`
         :heading-level: 3

      Each thread can queue common arguments in a few nanoseconds using its own lock-free queue.

   .. container:: quill-card
      :name: millions-of-messages-per-second

      .. rubric:: :si-icon:`octicons/pulse-16` :doc:`Backend throughput <benchmarks_throughput>`
         :heading-level: 3

      One worker merges available records by timestamp and can format and write millions of messages per second.

   .. container:: quill-card
      :name: macros-or-functions

      .. rubric:: :si-icon:`octicons/code-16` :doc:`Macros or functions <macro_free_mode>`
         :heading-level: 3

      :doc:`Logging macros <logging_macros>` skip argument evaluation at disabled
      levels. Use ``quill::info()`` for plain function-call syntax.

   .. container:: quill-card
      :name: fast-to-compile

      .. rubric:: :si-icon:`octicons/file-code-16` :doc:`Fast compilation <benchmarks_compile_time>`
         :heading-level: 3

      Log sites need only ``Logger.h`` and ``LogMacros.h``. Calls with the same
      argument types share their generated code.

   .. container:: quill-card
      :name: extend-types-and-destinations

      .. rubric:: :si-icon:`octicons/package-16` :ref:`Custom C++ types <recipes:Logging User Defined Types>`
         :heading-level: 3

      Include a ``quill/std`` header to log STL containers. Supply a formatter
      and a ``Codec`` specialization for a type of your own.

   .. container:: quill-card
      :name: add-context-and-control

      .. rubric:: :si-icon:`octicons/stack-16` :doc:`Context and diagnostics <mdc>`
         :heading-level: 3

      Attach thread-local fields such as a request ID to subsequent records. Keep
      recent events in a :doc:`backtrace buffer <backtrace_logging>` to flush on demand.

   .. container:: quill-card
      :name: highly-customizable

      .. rubric:: :si-icon:`octicons/sliders-16` :doc:`Queue and backend options <frontend_options>`
         :heading-level: 3

      Choose blocking or dropping queues, tune :doc:`worker affinity and flushing <backend_options>`,
      and apply :doc:`per-sink filters <filters>`.

   .. container:: quill-card
      :name: tested-for-production

      .. rubric:: :si-icon:`octicons/shield-check-16` :doc:`Cross-platform builds <installing>`
         :heading-level: 3

      Quill runs CI on Linux, macOS, Windows and BSD, with sanitizers and fuzzing.

.. _logs-metrics-and-data-through-one-backend:

One backend thread for every output
-----------------------------------

A trading system can log decisions, publish order latencies, write fills to CSV and capture exchange traffic
on one worker.

.. container:: quill-card-grid

   .. container:: quill-card
      :name: emit-structured-data

      .. rubric:: :si-icon:`octicons/log-16` :doc:`Text and JSON logs <sink_types>`
         :heading-level: 3

      Send the same event to a rotating text file and a :doc:`JSON sink <json_logging>`
      with named fields. Add a :doc:`custom sink <sinks>` for another destination.

   .. container:: quill-card

      .. rubric:: :si-icon:`octicons/graph-16` :doc:`Metrics <metrics>`
         :heading-level: 3

      Register counters, gauges or histograms once, then publish samples to the
      built-in Prometheus sink or your own metrics collector.

   .. container:: quill-card

      .. rubric:: :si-icon:`octicons/table-16` :doc:`CSV files <csv_writing>`
         :heading-level: 3

      Give a CSV writer a header such as ``city,temperature,humidity`` and
      append rows such as ``Athens,21.4,40`` from your application.

   .. container:: quill-card

      .. rubric:: :si-icon:`octicons/file-binary-16` :doc:`Binary data and pcap <binary_protocols>`
         :heading-level: 3

      Record raw bytes, decode protocol fields on the backend, or save
      :ref:`packet captures <binary_protocols:Writing pcap Files>` for Wireshark and tcpdump.

Start Here
----------

.. rst-class:: quill-start-links

- :si-icon:`octicons/download-16` :doc:`Install Quill <installing>` *Add Quill to your build with CMake or a package manager.*
- :si-icon:`octicons/rocket-16` :doc:`Write your first logger <quick_start>` *Create a logger, write to the console or a file and log your first message.*
- :si-icon:`octicons/book-16` :doc:`Browse the guides <guides>` *Sinks, formatting, filters, backend options and every other feature in depth.*
- :si-icon:`octicons/light-bulb-16` :doc:`Find a recipe <recipes>` *Short, complete solutions to common logging tasks.*
- :si-icon:`octicons/question-16` :doc:`Read the FAQ <faq>` *Answers to the questions users ask most.*
- :si-icon:`octicons/graph-16` :doc:`Compare benchmarks <benchmarks>` *Latency, throughput and compile time against other C++ logging libraries.*

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
   :caption: Benchmarks
   :hidden:

   benchmarks

.. toctree::
   :maxdepth: 2
   :caption: API
   :hidden:

   users-api
