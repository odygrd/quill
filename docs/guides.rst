.. title:: Guides

Guides
======

Task-focused guides for configuring Quill beyond the initial setup.

Use these pages when you want to tune backend/frontend behavior, add structured output,
customize sinks and formatting, publish metrics, or integrate more advanced asynchronous
flows into a larger codebase.

- **Core behavior:** :doc:`Backend Options <backend_options>`, :doc:`Frontend Options <frontend_options>`, :doc:`Loggers <loggers>`, :doc:`Log Levels <log_levels>`, :doc:`Logging Macros <logging_macros>`
- **Output, sinks, and metrics:** :doc:`Sinks <sinks>`, :doc:`Metrics <metrics>`, :doc:`Sink Types <sink_types>`, :doc:`Formatters <formatters>`, :doc:`Timestamp Types <timestamp_types>`, :doc:`Wide Strings <wide_strings>`
- **Structured and filtered logging:** :doc:`JSON Logging <json_logging>`, :doc:`CSV Writing <csv_writing>`, :doc:`Filters <filters>`, :doc:`Log Tagging <log_tagging>`, :doc:`Mapped Diagnostic Context (MDC) <mdc>`
- **Specialized flows:** :doc:`Binary Protocols <binary_protocols>`, :doc:`Backtrace Logging <backtrace_logging>`, :doc:`Manual Backend Worker <manual_backend_worker>`, :doc:`Macro Free Mode <macro_free_mode>`

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
