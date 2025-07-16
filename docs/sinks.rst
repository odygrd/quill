.. title:: Sinks

Sinks
=====

Sinks are objects responsible for writing logs to their respective targets.

A :cpp:class:`Sink` object serves as the base class for various sink-derived classes.

Each sink handles outputting logs to a single target, such as a file, console, or database.

Upon creation, a sink object is registered and owned by a central manager object, the `SinkManager`.

For files, one sink is created per filename, and the file is opened once. If a sink is requested that refers to an already opened file, the existing Sink object is returned.

When creating a logger, one or more sinks for that logger can be specified. Sinks can only be registered during the logger creation.

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

.. literalinclude:: examples/quill_docs_example_multiple_sinks.cpp
   :language: cpp
   :linenos:

2. Using a single ``Logger`` instance with multiple ``Sinks`` and tag-based filtering:

   This approach centralizes logging through a single logger while still directing messages to different destinations.

.. literalinclude:: examples/quill_docs_example_multiple_sinks_tags.cpp
   :language: cpp
   :linenos: