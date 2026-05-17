.. title:: Sinks

Sinks
=====

Use this page to understand how sinks are created, shared between loggers, and extended with custom implementations.

Sinks are objects responsible for writing logs to their respective targets.

A :cpp:class:`Sink` object serves as the base class for various sink-derived classes.

Each sink handles outputting logs to a single target, such as a file, console, or database.

Upon creation, a sink object is registered and owned by a central manager object, the `SinkManager`.

For files, one sink is created per normalized file path, and the file is opened once. If a sink is
requested that refers to an already opened file, the existing Sink object is returned and any
additional constructor arguments are ignored.

When creating a logger, one or more sinks for that logger can be specified. Sinks can only be registered during the logger creation.

Sinks can be obtained using :cpp:func:`FrontendImpl::get_sink`, :cpp:func:`FrontendImpl::create_or_get_sink`, or :cpp:func:`FrontendImpl::create_sink`.

- ``create_sink`` creates a new sink and throws ``QuillError`` if a sink with the same name already exists.
- ``create_or_get_sink`` creates a new sink if one does not already exist; otherwise, it returns the existing sink with the specified name. Note that when a sink with the given name already exists, the provided constructor arguments are ignored — the existing sink is returned as-is.
- ``get_sink`` retrieves an existing sink by name and throws ``QuillError`` if it does not exist.

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
