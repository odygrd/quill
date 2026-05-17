.. title:: Backtrace Logging

Backtrace Logging
=================

Use this page to capture low-level log messages in a ring buffer and flush them on demand or when a high-severity event occurs.

Backtrace logging enables log messages to be stored in a ring buffer and can be:

- Displayed later on demand
- Automatically flushed when a high severity log message is logged

To enable backtrace logging, initialize it first with an appropriate log level use :cpp:func:`LoggerImpl::init_backtrace`.
By default, backtrace logs are automatically flushed when an error occurs.

To manually flush backtrace logs, call :cpp:func:`LoggerImpl::flush_backtrace`.

.. note::
   Backtrace log messages are always pushed to the SPSC (Single Producer Single Consumer) queue from the frontend to the backend. They are stored in a ring buffer that resides in the backend and flushed as needed.

.. note::
   Backtrace log messages store the original timestamp of the message. Since they are stored and flushed later, the timestamps in the log file may appear out of order. This remains true even when :cpp:member:`BackendOptions::ensure_monotonic_output_timestamps` is enabled.

Store messages in the ring buffer and display them when ``LOG_ERROR`` is logged
-------------------------------------------------------------------------------

.. literalinclude:: snippets/quill_docs_example_backtrace_logging_1.cpp
   :language: cpp
   :linenos:

Store messages in the ring buffer and display them on demand
------------------------------------------------------------

.. literalinclude:: snippets/quill_docs_example_backtrace_logging_2.cpp
   :language: cpp
   :linenos:
