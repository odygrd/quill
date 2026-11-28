.. title:: Formatters

Formatters
==========

Set :cpp:class:`PatternFormatterOptions` when creating a :cpp:class:`LoggerImpl` to choose its log layout,
timestamp and source-location display. Each logger has its own :cpp:class:`PatternFormatter` and uses the
default pattern unless you supply one. Combine the attributes below to build a pattern.

.. list-table::
   :header-rows: 1
   :widths: 22 22 56

   * - Name
     - Format
     - Description
   * - ``time``
     - ``%(time)``
     - Human-readable timestamp when the log statement was issued.
   * - ``file_name``
     - ``%(file_name)``
     - Filename only, excluding the full path.
   * - ``full_path``
     - ``%(full_path)``
     - Full path of the source file where the logging call was issued.
   * - ``caller_function``
     - ``%(caller_function)``
     - Name of the function containing the logging call.
   * - ``log_level``
     - ``%(log_level)``
     - Log level description: ``TRACE_L3``, ``TRACE_L2``, ``TRACE_L1``, ``DEBUG``, ``INFO``,
       ``NOTICE``, ``WARNING``, ``ERROR``, ``CRITICAL``, or ``BACKTRACE``.
   * - ``log_level_short_code``
     - ``%(log_level_short_code)``
     - Abbreviated log level: ``T3``, ``T2``, ``T1``, ``D``, ``I``, ``N``, ``W``, ``E``,
       ``C``, or ``BT``.
   * - ``line_number``
     - ``%(line_number)``
     - Source line number where the logging call was issued.
   * - ``logger``
     - ``%(logger)``
     - Name of the logger.
   * - ``message``
     - ``%(message)``
     - The log message.
   * - ``thread_id``
     - ``%(thread_id)``
     - Thread ID, if available.
   * - ``thread_name``
     - ``%(thread_name)``
     - Thread name, if set. The name of the thread must be set before issuing any log
       statement on that thread.
   * - ``process_id``
     - ``%(process_id)``
     - Process ID.
   * - ``source_location``
     - ``%(source_location)``
     - Source file path and line number as a single string. Use
       ``source_location_path_strip_prefix`` and
       ``source_location_remove_relative_paths`` in ``PatternFormatterOptions`` to
       customize the displayed path.
   * - ``short_source_location``
     - ``%(short_source_location)``
     - Filename and line number as a single string formatted as ``filename:line``,
       excluding the full file path.
   * - ``tags``
     - ``%(tags)``
     - Additional custom tags supplied by ``_TAGS`` macros, macro-free logging, or runtime
       metadata.
   * - ``mdc``
     - ``%(mdc)``
     - Mapped Diagnostic Context fields for the current frontend thread. This expands to an
       empty string when no MDC fields are set.
   * - ``named_args``
     - ``%(named_args)``
     - Key-value pairs appended to the message. This is only applicable to a named-args
       format string and remains empty otherwise.

Single literal braces work in patterns. The {fmt} spellings ``{{`` and ``}}`` each produce one brace.
A nested JSON pattern such as ``{"app":{"msg":"%(message)"}}`` prints one closing brace. To print both,
end the pattern with ``}}}}`` or double every brace.

Customising the timestamp
-------------------------

The default is local time with ``"%H:%M:%S.%Qns"``. Select local or GMT and combine the platform's
``strftime()`` specifiers with at most one of the fractional-second specifiers below.

========= ============
Specifier Description
========= ============
%Qms      Milliseconds
%Qus      Microseconds
%Qns      Nanoseconds
========= ============

A pattern may use one of them, once. Repeating one or combining two prevents the backend from creating the
logger's formatter, so the logger's records are not written; each reports the error through
``BackendOptions::error_notifier``, which suppresses a repeat of its last error for five seconds unless a
different error intervenes. ``QUILL_NO_EXCEPTIONS`` builds terminate on the first affected record.

Use ``%%`` for a literal percent sign, including ``%%Qms`` or ``%%T``. Unsupported ``strftime()`` patterns
produce a formatting error. Support depends on the platform's C runtime.

**Timestamp caching**

- Quill caches plain ``%H``, ``%I``, ``%k``, ``%l``, ``%M``, ``%S``, ``%s``, ``%R`` and ``%T``.
- ``%X``, ``%EX``, ``%c``, ``%Ec``, ``%r``, BSD ``%+`` and time-of-day flags, widths or ``E``/``O`` modifiers
  (e.g. ``%-H``, ``%_M``, ``%10S``, ``%OH``) require a slower ``strftime()`` call per timestamp.
  The backend reports this once through ``error_notifier``. Date specifiers such as ``%-d`` are unaffected.
- For ``Timezone::GmtTime``, use plain ``%s``. A flagged form such as ``%10s`` uses local time in ``strftime()``
  and differs by the UTC offset.

Customizing Log Message Formats
-------------------------------

.. literalinclude:: snippets/quill_docs_example_custom_format.cpp
   :language: cpp
   :linenos:

Advanced Pattern Formatter Configuration
-----------------------------------------

**Source paths.** Strip prefixes or normalize relative components in ``%(source_location)`` (path and line).

.. code-block:: cpp

   quill::PatternFormatterOptions options;
   
   // Strip common path prefixes to shorten source locations
   options.source_location_path_strip_prefix = "/home/user/project/";
   // "/home/user/project/src/main.cpp:42" becomes "src/main.cpp:42"
   
   // Remove relative path components like "../"
   options.source_location_remove_relative_paths = true;
   // "../../src/utils/../main.cpp:15" becomes "src/main.cpp:15"

``%(short_source_location)`` always shows just the filename and line.

**Function names.** ``%(caller_function)`` normally shows a simple function name. Define
``QUILL_DETAILED_FUNCTION_NAME`` before including Quill headers or as a compiler flag for the full signature.

.. code-block:: cpp

   #define QUILL_DETAILED_FUNCTION_NAME
   #include "quill/LogMacros.h"
   
   // Or compile with: -DQUILL_DETAILED_FUNCTION_NAME

Compiler-specific signatures such as ``__PRETTY_FUNCTION__`` can include return type, namespaces, class and
parameter types. Use ``process_function_name`` to shorten them.

.. code-block:: cpp

   quill::PatternFormatterOptions options;
   
   // Custom function to extract just the function name from detailed signatures
   options.process_function_name = [](const char* detailed_name) -> std::string_view {
       // Example: Extract "myFunction" from "void MyClass::myFunction(int, const std::string&)"
       // Implementation depends on your compiler and requirements
       std::string_view name(detailed_name);
       // ... custom parsing logic to extract just the function name ...
       return name;
   };

The callback is active only with ``QUILL_DETAILED_FUNCTION_NAME``.

**Tags.** Set ``process_tags`` to transform the text inserted into ``%(tags)``.

.. code-block:: cpp

   quill::PatternFormatterOptions options{"%(tags)%(message)"};
   options.process_tags = [](char const* tags) -> std::string {
       std::string result{tags};
       // For example, map a runtime numeric identifier to a readable name.
       // ... modify result ...
       return result;
   };

Use a regular function or non-capturing lambda (the option stores a function pointer). The callback runs
only for a tagged record when the pattern includes ``%(tags)``. Sinks still receive the original metadata.
See :doc:`log_tagging` for the tag APIs.

**Timestamps.** Set the timezone and pattern together.

.. code-block:: cpp

   quill::PatternFormatterOptions options;
   
   // Use GMT timezone instead of local time
   options.timestamp_timezone = quill::Timezone::GmtTime;
   
   // Customize timestamp format (same as strftime + fractional specifiers)
   options.timestamp_pattern = "%Y-%m-%d %H:%M:%S.%Qms";  // Include date and milliseconds

**Multi-line messages.** Choose whether continuation lines repeat the metadata.

.. code-block:: cpp

   quill::PatternFormatterOptions options;
   
   // Disable metadata on continuation lines for cleaner multi-line output
   options.add_metadata_to_multi_line_logs = false;
   
   // With add_metadata_to_multi_line_logs = true (default):
   // 2024-01-15 10:30:45.123 [1234] main.cpp:42 LOG_INFO     MyLogger Line 1
   // 2024-01-15 10:30:45.123 [1234] main.cpp:42 LOG_INFO     MyLogger Line 2
   
   // With add_metadata_to_multi_line_logs = false:
   // 2024-01-15 10:30:45.123 [1234] main.cpp:42 LOG_INFO     MyLogger Line 1
   //                                                                   Line 2
