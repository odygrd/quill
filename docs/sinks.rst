.. title:: Sinks

Sinks
=====

Use this page to understand how sinks are created, shared between loggers, and extended with custom implementations.

Sinks are objects responsible for writing logs to their respective targets.

A :cpp:class:`Sink` object serves as the base class for various sink-derived classes.

Each sink handles outputting logs, metrics, or both to a single target, such as a file, console, database, or metrics backend.

Upon creation, a sink object is registered and owned by a central manager object, the `SinkManager`.

For files, one sink is created per normalized file path, and the file is opened once. If a
compatible sink type is requested for an already opened file, the existing Sink object is returned
and any additional constructor arguments are ignored. In RTTI-enabled builds, requesting the same
name with an incompatible type throws ``QuillError``. Existing file aliases on POSIX are resolved for
lookup, while the configured path is retained for file I/O.

When creating a logger, one or more sinks for that logger can be specified. Sinks can only be registered during the logger creation.

Sinks can be obtained using :cpp:func:`FrontendImpl::get_sink`, :cpp:func:`FrontendImpl::create_or_get_sink`, or :cpp:func:`FrontendImpl::create_sink`.

- ``create_sink`` creates a new sink and throws ``QuillError`` if a sink with the same name already exists.
- ``create_or_get_sink`` creates a new sink if one does not already exist; otherwise, it returns a
  compatible existing sink and ignores the provided constructor arguments. In RTTI-enabled builds,
  an incompatible existing type throws ``QuillError``.
- ``get_sink`` retrieves an existing sink by name and throws ``QuillError`` if it does not exist.

In builds configured with ``QUILL_NO_EXCEPTIONS``, conditions documented as throwing ``QuillError``
invoke Quill's fatal error path instead. An I/O failure of a file, stream or systemd sink on the
backend thread, such as a write to a full disk or a file that cannot be opened again, is the
exception: it is reported through ``BackendOptions::error_notifier`` in these builds too, and the
backend keeps running. A file sink that is destroyed on another thread, e.g. as the process exits,
gives up the records that it cannot write.

.. warning::

   Builds without RTTI cannot diagnose an incompatible type passed to ``create_or_get_sink``.
   In those builds, request a given sink name only with the same sink type, or with a compatible
   base type.

.. warning::

   A sink name remains reserved while that sink is being constructed. A ``FileEventNotifier``
   callback invoked during construction must not call ``create_sink`` or ``create_or_get_sink``
   with that same sink name, because the recursive call waits for its own construction to finish.
   Creating a sink with a different name from the callback is supported.

Configuring Sinks
-----------------

Some sinks accept a configuration object (e.g. ``FileSinkConfig``, ``SyslogSinkConfig``). These are
passed as additional constructor arguments to the ``create_or_get_sink`` or ``create_sink`` call.
A common pattern is to use a lambda to construct the config inline:

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
It is possible to share the same `Sink` object between multiple `Logger` objects. For example, when all logger objects are writing to the same file. The following code is also thread-safe.

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
You can extend the library by creating and integrating your own ``Sink`` types. The code within the ``Sink`` class is executed by a single backend worker thread.

This can be useful if you want to direct log output to alternative destinations, such as a database, a network service, or even to write ``Parquet`` files.

A custom sink implements ``write_log(LogRecord const&)`` and ``flush_sink(SinkFlushReason)``.
``LogRecord`` holds the formatted message and statement, the metadata, thread, logger, level,
named arguments and MDC of the record. Its views are valid only during the call, so copy anything
the sink keeps. The flush reason distinguishes backend idle flushes, explicit
``logger->flush_log()`` flushes, and the final flush at backend shutdown.

To export metrics, also implement ``write_metric(MetricRecord const&)`` and bind a logger to that
sink. The default ``Sink::write_metric()`` is a no-op, so log-only sinks need not implement it.
See :doc:`Metrics <metrics>` for the metric publishing model and the Prometheus/custom-sink
examples.

To receive the bytes logged with ``LOG_BINARY``, implement ``write_binary(BinaryRecord const&)``.
The default ``Sink::write_binary()`` is a no-op as well, and the stream and file sinks write the
bytes as they are. See :doc:`Binary Protocols <binary_protocols>`.

.. literalinclude:: ../examples/user_defined_sink.cpp
   :language: cpp
   :linenos:

Routing Log Messages to Multiple Sinks
--------------------------------------

The library provides multiple approaches for routing log messages to different sinks. The following examples demonstrate two common patterns using ``FileSink``, but the same principles apply when using custom ``Sinks`` and ``Filters``.

1. Using multiple ``Logger`` instances, each bound to a specific ``Sink``:

   This approach is straightforward and provides clear separation of log streams.

.. literalinclude:: snippets/quill_docs_example_multiple_sinks.cpp
   :language: cpp
   :linenos:

2. Using a single ``Logger`` instance with multiple ``Sinks`` and tag-based filtering:

   This approach centralizes logging through a single logger while still directing messages to different destinations.

.. literalinclude:: snippets/quill_docs_example_multiple_sinks_tags.cpp
   :language: cpp
   :linenos:
