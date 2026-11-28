.. title:: Loggers

Loggers
=======

Create a named ``Logger`` through the frontend, then share it across logging threads. Choose its sinks,
pattern and clock at creation. To change those settings, wait for removal, recreate the logger and replace
every cached pointer. The new object can have a different address.


Logger Creation
---------------

.. code:: cpp

     auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

     quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

     LOG_INFO(logger, "Hello from {}", "library foo");

Logger Retrieval
----------------

- :cpp:func:`FrontendImpl::create_logger` requires a new name and throws ``QuillError`` if it is taken.
- :cpp:func:`FrontendImpl::create_or_get_logger` returns the existing logger for a known name, ignoring new
  sinks, pattern, clock and other creation arguments. Otherwise it creates the logger.
- :cpp:func:`FrontendImpl::get_logger` looks up a logger by name and returns ``nullptr`` on a miss.

.. code:: cpp

    quill::Logger* logger = quill::Frontend::get_logger("root");

Logger Removal
--------------

Stop all users of a ``Logger*`` before requesting removal. Assign one thread to remove each logger. Concurrent
removal of different loggers is supported. Removing the same logger from multiple threads is undefined behavior.
Unique names per application thread can simplify ownership.

:cpp:func:`FrontendImpl::remove_logger` requests asynchronous removal. The worker processes pending statements
and flushes the logger's sinks before completing removal. :cpp:func:`FrontendImpl::remove_logger_blocking`
waits for completion. Use it only while the backend is running and outside backend callbacks. Once removal
completes, you can reuse the name with different settings.

To use a different configuration, you can also create a logger with a new name. Remove an old logger when
you need to release its sinks. For example, removing a TCP session's logger on disconnect closes its file
once all loggers and other ``shared_ptr`` owners release the sink. Owners that keep the file open still see
the removal flush.

.. literalinclude:: snippets/quill_docs_example_loggers_remove.cpp
   :language: cpp
   :linenos:

.. literalinclude:: ../examples/logger_removal_with_file_event_notifier.cpp
   :language: cpp
   :linenos:

The file-removal example uses :cpp:class:`FileEventNotifier`. See :ref:`sink_types:file callbacks` for the
platform handle type, callback threading and the effect of ``before_write`` on size-based rotation.

Simplifying Logger Usage for Single Root Logger Applications
------------------------------------------------------------

For a single root logger, store the pointer in a static variable and wrap the macros to supply it, as in the
`example <https://github.com/odygrd/quill/blob/master/examples/recommended_usage/quill_static_lib/overwrite_macros.h>`_.
