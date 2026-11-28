.. title:: Backtrace Logging

Backtrace Logging
=================

Keep recent low-level messages in a ring buffer to see what led up to an error.
:cpp:func:`LoggerImpl::init_backtrace` takes a capacity and an optional severity that triggers output.
Use ``LogLevel::Error`` to flush on errors and above, or keep the default ``LogLevel::None`` and call
:cpp:func:`LoggerImpl::flush_backtrace` for on-demand output.

The frontend enqueues backtrace records and the backend stores them in the ring with their original
timestamps. Flushing later can put those timestamps out of order, even with
:cpp:member:`BackendOptions::ensure_monotonic_output_timestamps` enabled.

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
