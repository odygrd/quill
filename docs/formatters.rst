.. title:: Formatters

Formatters
==========

The :cpp:class:`PatternFormatter` specifies the layout of log records in the final output.

Each :cpp:class:`LoggerImpl` object has a ``PatternFormatter`` object.
This means that each Logger can be customised to output in a different format.

Customising the format output only be done during the creation of the logger via the :cpp:class:`PatternFormatterOptions`

If no custom format is set each newly created Sink uses the same formatting as the default logger.

The format output can be customised by providing a string of certain attributes.

+-------------------------+--------------------------+----------------------------------------+
| Name                    | Format                   | Description                            |
+=========================+==========================+========================================+
| time                    | %(time)                  | Human-readable timestamp when the log  |
|                         |                          | statement was issued                   |
+-------------------------+--------------------------+----------------------------------------+
| file_name               | %(file_name)             | Filename only, excluding the full path |
+-------------------------+--------------------------+----------------------------------------+
| full_path               | %(full_path)             | Full path of the source file where the |
|                         |                          | logging call was issued                |
+-------------------------+--------------------------+----------------------------------------+
| caller_function         | %(caller_function)       | Name of function containing the        |
|                         |                          | logging call                           |
+-------------------------+--------------------------+----------------------------------------+
| log_level               | %(log_level)             | Log level description                  |
|                         |                          | (‘TRACEL3’, ‘TRACEL2’, ‘TRACEL1’,      |
|                         |                          | ‘DEBUG’, ‘INFO’, ‘WARNING’, ‘ERROR’,   |
|                         |                          | ‘CRITICAL’, ‘BACKTRACE’)               |
+-------------------------+--------------------------+----------------------------------------+
| log_level_short_code    | %(log_level_short_code)  | Abbreviated log level (‘T3’, ‘T2’,     |
|                         |                          | ‘T1’, ‘D’, ‘I’, ‘W’, ‘E’, ‘C’, ‘BT’)   |
+-------------------------+--------------------------+----------------------------------------+
| line_number             | %(line_number)           | Source line number where the logging   |
|                         |                          | call was issued                        |
+-------------------------+--------------------------+----------------------------------------+
| logger                  | %(logger)                | Name of the logger                     |
+-------------------------+--------------------------+----------------------------------------+
| message                 | %(message)               | The log message                        |
+-------------------------+--------------------------+----------------------------------------+
| thread_id               | %(thread_id)             | Thread ID, if available                |
+-------------------------+--------------------------+----------------------------------------+
| thread_name             | %(thread_name)           | Thread name, if set. The name of the   |
|                         |                          | thread must be set prior to issuing    |
|                         |                          | any log statement on that thread       |
+-------------------------+--------------------------+----------------------------------------+
| process_id              | %(process_id)            | Process ID                             |
+-------------------------+--------------------------+----------------------------------------+
| source_location         | %(source_location)       | Source file path and line number       |
|                         |                          | as a single string. See                |
|                         |                          | `source_location_path_depth`           |
|                         |                          | `PatternFormatterOptions`              |
|                         |                          | to control directory depth             |
+-------------------------+--------------------------+----------------------------------------+
| short_source_location   | %(short_source_location) | Filename and line number as a single   |
|                         |                          | string formatted as filename:line,     |
|                         |                          | excluding the full file path           |
+-------------------------+--------------------------+----------------------------------------+
| tags                    | %(tags)                  | Additional custom tags appended to the |
|                         |                          | message when _TAGS macros are used     |
+-------------------------+--------------------------+----------------------------------------+
| named_args              | %(named_args)            | Key-value pairs appended to the        |
|                         |                          | message. Only applicable with          |
|                         |                          | for a named args format string;        |
|                         |                          | remains empty otherwise                |
+-------------------------+--------------------------+----------------------------------------+

Customising the timestamp
-------------------------

The timestamp is customisable by :

- Format. Same format specifiers as ``strftime(...)`` format without the additional ``.Qms`` ``.Qus`` ``.Qns`` arguments.
- Local timezone or GMT timezone. Local timezone is used by default.
- Fractional second precision. Using the additional fractional second specifiers in the timestamp format string.

========= ============
Specifier Description
========= ============
%Qms      Milliseconds
%Qus      Microseconds
%Qns      Nanoseconds
========= ============

By default ``"%H:%M:%S.%Qns"`` is used.

.. note:: MinGW does not support all ``strftime(...)`` format specifiers and you might get a ``bad alloc`` if the format specifier is not supported

Customizing Log Message Formats
-------------------------------

.. literalinclude:: examples/quill_docs_example_custom_format.cpp
   :language: cpp
   :linenos:

Advanced Pattern Formatter Configuration
-----------------------------------------

The :cpp:class:`PatternFormatterOptions` class provides additional configuration options beyond the basic format pattern:

**Source Location Path Customization**

You can customize how source file paths are displayed when using the ``%(source_location)`` attribute (full path + line number):

.. code-block:: cpp

   quill::PatternFormatterOptions options;
   
   // Strip common path prefixes to shorten source locations
   options.source_location_path_strip_prefix = "/home/user/project/";
   // "/home/user/project/src/main.cpp:42" becomes "src/main.cpp:42"
   
   // Remove relative path components like "../"
   options.source_location_remove_relative_paths = true;
   // "../../src/utils/../main.cpp:15" becomes "src/main.cpp:15"

.. note::
   These options only affect the ``%(source_location)`` attribute, which shows the full file path. The ``%(short_source_location)`` attribute (filename only + line number) is not affected by these settings.

**Custom Function Name Processing**

By default, the ``%(caller_function)`` attribute shows basic function names. However, you can enable detailed function signatures by defining ``QUILL_DETAILED_FUNCTION_NAME`` before including Quill headers or as a compiler flag:

.. code-block:: cpp

   #define QUILL_DETAILED_FUNCTION_NAME
   #include "quill/LogMacros.h"
   
   // Or compile with: -DQUILL_DETAILED_FUNCTION_NAME

When ``QUILL_DETAILED_FUNCTION_NAME`` is enabled, the logger uses compiler-specific macros (like ``__PRETTY_FUNCTION__``) to provide full function signatures including return types, namespaces, class names, and parameter types. Since these detailed signatures can be verbose and compiler-specific, you can provide a custom processing function to format them:

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

.. note::
   The ``process_function_name`` callback is only used when ``QUILL_DETAILED_FUNCTION_NAME`` is enabled. Without this define, ``%(caller_function)`` shows simple function names and this callback is ignored.

**Timestamp Configuration**

Control timestamp timezone and format:

.. code-block:: cpp

   quill::PatternFormatterOptions options;
   
   // Use GMT timezone instead of local time
   options.timestamp_timezone = quill::Timezone::GmtTime;
   
   // Customize timestamp format (same as strftime + fractional specifiers)
   options.timestamp_pattern = "%Y-%m-%d %H:%M:%S.%Qms";  // Include date and milliseconds

**Multi-line Message Handling**

Control whether metadata is added to each line of multi-line log messages:

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
