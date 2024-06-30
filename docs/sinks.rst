.. title:: Sinks

Sinks
=====

Sinks are objects responsible for writing logs to their respective targets.

A :cpp:class:`quill::Sink` object serves as the base class for various sink-derived classes.

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