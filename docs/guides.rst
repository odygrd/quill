.. title:: Guides

Guides
======

Choose a guide when you need to configure Quill beyond the initial setup, add an output or integrate logging with your application.

.. rubric:: Core behavior
   :heading-level: 2
   :class: quill-group

.. rst-class:: quill-doc-cards

- :si-icon:`octicons/server-16` :doc:`Backend Options <backend_options>` *Tune the worker at runtime.*
- :si-icon:`octicons/sliders-16` :doc:`Frontend Options <frontend_options>` *Choose queue and memory policies at compile time.*
- :si-icon:`octicons/log-16` :doc:`Loggers <loggers>` *Create, retrieve, configure, and remove loggers.*
- :si-icon:`octicons/meter-16` :doc:`Log Levels <log_levels>` *Set runtime and compile-time thresholds.*
- :si-icon:`octicons/code-16` :doc:`Logging Macros <logging_macros>` *Look up LOG\_, LOGV\_, LOGJ\_ and dynamic calls.*

.. rubric:: Output, sinks, and metrics
   :heading-level: 2
   :class: quill-group

.. rst-class:: quill-doc-cards

- :si-icon:`octicons/plug-16` :doc:`Sinks <sinks>` *Create, share or implement a sink.*
- :si-icon:`octicons/graph-16` :doc:`Metrics <metrics>` *Publish to Prometheus or a custom collector.*
- :si-icon:`octicons/apps-16` :doc:`Sink Types <sink_types>` *Choose a built-in destination.*
- :si-icon:`octicons/typography-16` :doc:`Formatters <formatters>` *Set the layout of lines, times and source locations.*
- :si-icon:`octicons/clock-16` :doc:`Timestamp Types <timestamp_types>` *Choose TSC, system or custom clocks.*
- :si-icon:`octicons/globe-16` :doc:`Wide Strings <wide_strings>` *Log wchar_t strings on Windows.*

.. rubric:: Structured and filtered logging
   :heading-level: 2
   :class: quill-group

.. rst-class:: quill-doc-cards

- :si-icon:`octicons/code-square-16` :doc:`JSON Logging <json_logging>` *Write structured fields to a file or console alongside text logs.*
- :si-icon:`octicons/table-16` :doc:`CSV Writing <csv_writing>` *Write CSV files asynchronously on the backend thread, alongside your logs.*
- :si-icon:`octicons/filter-16` :doc:`Filters <filters>` *Route records by level, content or custom criteria.*
- :si-icon:`octicons/tag-16` :doc:`Log Tagging <log_tagging>` *Classify, filter or monitor records by tag.*
- :si-icon:`octicons/id-badge-16` :doc:`Mapped Diagnostic Context (MDC) <mdc>` *Carry request or session fields across log calls.*

.. rubric:: Specialized flows
   :heading-level: 2
   :class: quill-group

.. rst-class:: quill-doc-cards

- :si-icon:`octicons/file-binary-16` :doc:`Binary Protocols <binary_protocols>` *Decode protocol fields, record raw bytes or write pcap captures.*
- :si-icon:`octicons/history-16` :doc:`Backtrace Logging <backtrace_logging>` *Keep recent events for an error or on-demand dump.*
- :si-icon:`octicons/tools-16` :doc:`Manual Backend Worker <manual_backend_worker>` *Drive the worker from your own thread or event loop.*
- :si-icon:`octicons/file-code-16` :doc:`Macro-Free Mode <macro_free_mode>` *Use function-call syntax and check its costs.*

.. toctree::
   :maxdepth: 2
   :hidden:

   backend_options
   backtrace_logging
   binary_protocols
   csv_writing
   filters
   formatters
   frontend_options
   json_logging
   log_tagging
   mdc
   loggers
   log_levels
   logging_macros
   manual_backend_worker
   metrics
   sinks
   sink_types
   timestamp_types
   wide_strings
   macro_free_mode
