.. title:: Sinks

Sinks
=====

A :cpp:class:`Sink` sends logs, metrics or both to one destination, such as a file, console, database or
metrics service. Attach one or more sinks when creating a :doc:`logger <loggers>`; sinks cannot be added to it later. The central ``SinkManager``
registers sinks so loggers can share them.

- :cpp:func:`FrontendImpl::create_sink` creates a sink and throws ``QuillError`` on a duplicate name.
- :cpp:func:`FrontendImpl::create_or_get_sink` returns a compatible existing sink and ignores new constructor
  arguments, or creates it if absent. With RTTI, an incompatible type throws ``QuillError``. Without RTTI,
  request each name only with its original type or a compatible base.
- :cpp:func:`FrontendImpl::get_sink` retrieves a sink by name and throws ``QuillError`` if absent.

File sinks open each normalized path once. On POSIX, lookup resolves existing file aliases. File I/O uses
the configured path.

.. warning::

   A sink name is reserved during construction. A ``FileEventNotifier`` construction callback that calls
   ``create_sink`` or ``create_or_get_sink`` with that name waits for itself. It may create a differently named sink.

In ``QUILL_NO_EXCEPTIONS`` builds, conditions that throw ``QuillError`` usually invoke Quill's fatal error path.
Backend file, stream or systemd sink I/O failures (such as a full disk or a failed reopen) still go to
``BackendOptions::error_notifier``, and the worker continues. A file sink destroyed on another thread, including
at process exit, gives up records it cannot write.

Configuring Sinks
-----------------

Pass options such as ``FileSinkConfig`` or ``SyslogSinkConfig`` as extra arguments to ``create_sink`` or
``create_or_get_sink``. A lambda can build the configuration inline.

.. code:: cpp

     auto sink = quill::Frontend::create_or_get_sink<quill::SyslogSink>(
       "my_syslog",
       []()
       {
         quill::SyslogSinkConfig cfg;
         cfg.set_identifier("my_app");
         return cfg;
       }());

Sharing Sinks Between Loggers
-----------------------------
Loggers writing to the same file can share one sink. This setup is thread-safe.

.. code:: cpp

     auto file_sink = Frontend::create_or_get_sink<FileSink>(
       filename,
       []()
       {
         FileSinkConfig cfg;
         cfg.set_open_mode('w');
         return cfg;
       }(),
       FileEventNotifier{});

     quill::Logger* logger_a = Frontend::create_or_get_logger("logger_a", file_sink);
     quill::Logger* logger_b = Frontend::create_or_get_logger("logger_b", file_sink);

Customizing the Library with User-Defined Sinks
-----------------------------------------------
Implement ``write_log(LogRecord const&)`` and ``flush_sink(SinkFlushReason)`` to write to a network service,
database, Parquet file or another target. The worker calls these methods. ``LogRecord`` provides the formatted
message and statement, metadata, thread, logger, level, named arguments and MDC. Its views expire on return.
Copy fields the sink will retain.

The flush reason distinguishes idle flushes, explicit ``logger->flush_log()`` and backend shutdown.
Two optional callbacks let the sink handle other record types. Both default to doing nothing.

- ``write_metric(MetricRecord const&)`` receives samples when a logger uses the sink. See :doc:`metrics`
  for Prometheus and custom exporters.
- ``write_binary(BinaryRecord const&)`` receives ``LOG_BINARY`` payloads. Stream and file sinks write the bytes
  verbatim. See :doc:`binary_protocols`.

.. literalinclude:: ../examples/user_defined_sink.cpp
   :language: cpp
   :linenos:

Routing Log Messages to Multiple Sinks
--------------------------------------

These ``FileSink`` examples also work with custom sinks and filters.

1. Give each log stream its own logger and sink.

.. literalinclude:: snippets/quill_docs_example_multiple_sinks.cpp
   :language: cpp
   :linenos:

2. Use one logger with several sinks, and route messages by tag.

.. literalinclude:: snippets/quill_docs_example_multiple_sinks_tags.cpp
   :language: cpp
   :linenos:
