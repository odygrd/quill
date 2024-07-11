.. title:: Formatters

Formatters
==========

The :cpp:class:`quill::PatternFormatter` specifies the layout of log records in the final output.

Each :cpp:class:`quill::LoggerImpl` object owns a ``PatternFormatter`` object.
This means that each Logger can be customised to output in a different format.

Customising the format output only be done during the creation of the logger.

If no custom format is set each newly created Sink uses the same formatting as the default logger.

The format output can be customised by providing a string of certain
attributes.

+-------------------------+--------------------------+----------------------------------------+
| Name                    | Format                   | Description                            |
+=========================+==========================+========================================+
| time                    | %(time)                  | Human-readable time when the LogRecord |
|                         |                          | was created. By default this is of the |
|                         |                          | form '2003-07-08 16:49:45.896' (the    |
|                         |                          | numbers after the period are the       |
|                         |                          | millisecond portion of the time).      |
+-------------------------+--------------------------+----------------------------------------+
| file_name               | %(file_name)             | Filename portion of pathname.          |
+-------------------------+--------------------------+----------------------------------------+
| full_path               | %(full_path)             | Full path of the source file where the |
|                         |                          | logging call was issued.               |
+-------------------------+--------------------------+----------------------------------------+
| caller_function         | %(caller_function)       | Name of function containing the        |
|                         |                          | logging call.                          |
+-------------------------+--------------------------+----------------------------------------+
| log_level               | %(log_level)             | Text logging level for the message     |
|                         |                          | (‘TRACEL3’, ‘TRACEL2’, ‘TRACEL1’,      |
|                         |                          | ‘DEBUG’, ‘INFO’, ‘WARNING’, ‘ERROR’,   |
|                         |                          | ‘CRITICAL’, ‘BACKTRACE’).              |
+-------------------------+--------------------------+----------------------------------------+
| log_level_short_code    | %(log_level_short_code)  | Abbreviated log level (‘T3’, ‘T2’,     |
|                         |                          | ‘T1’, ‘D’, ‘I’, ‘W’, ‘E’, ‘C’, ‘BT’).  |
+-------------------------+--------------------------+----------------------------------------+
| line_number             | %(line_number)           | Source line number where the logging   |
|                         |                          | call was issued (if available).        |
+-------------------------+--------------------------+----------------------------------------+
| logger                  | %(logger)                | Name of the logger used to log the     |
|                         |                          | call.                                  |
+-------------------------+--------------------------+----------------------------------------+
| message                 | %(message)               | The logged message, computed as msg %  |
|                         |                          | args. This is set when Formatter.      |
|                         |                          | format() is invoked.                   |
+-------------------------+--------------------------+----------------------------------------+
| thread_id               | %(thread_id)             | Thread ID (if available).              |
+-------------------------+--------------------------+----------------------------------------+
| thread_name             | %(thread_name)           | Thread name if set. The name of the    |
|                         |                          | thread must be set prior to issuing    |
|                         |                          | any log statement on that thread.      |
+-------------------------+--------------------------+----------------------------------------+
| process_id              | %(process_id)            | Process ID                             |
+-------------------------+--------------------------+----------------------------------------+
| source_location         | %(source_location)       | Full source file path and line number  |
|                         |                          | as a single string                     |
+-------------------------+--------------------------+----------------------------------------+
| short_source_location   | %(short_source_location) | Full source file path and line         |
|                         |                          | number as a single string              |
+-------------------------+--------------------------+----------------------------------------+
| tags                    | %(tags)                  | Additional custom tags appended to the |
|                         |                          | message when _WITH_TAGS macros are     |
|                         |                          | used.                                  |
+-------------------------+--------------------------+----------------------------------------+
| named_args              | %(named_args)            | Key-value pairs appended to the        |
|                         |                          | message. Only applicable with          |
|                         |                          | for a named args log format;           |
|                         |                          | remains empty otherwise.               |
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

.. code:: cpp

  quill::Logger* logger =
    quill::Frontend::create_or_get_logger("root", std::move(sink),
                                          "%(time) [%(thread_id)] %(short_source_location:<28) "
                                          "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                          "%H:%M:%S.%Qns", quill::Timezone::GmtTime);