.. title:: Loggers

Loggers
=======

``Logger`` instances are created by users with a specified name, along with designated Sinks and a Formatter. Direct instantiation of logger objects is not possible; instead, they must first be created with the desired configurations.

Upon creation, users specify the Sinks and Formatter for the logger. The logger name serves as an identifier, allowing the same logger configuration to be reused without re-specifying Sinks and Formatter settings.

The Logger class is thread-safe.

Loggers can be removed using :cpp:class:`quill::FrontendImpl::remove_logger` which deletes them asynchronously.

When all loggers associated with a particular ``Sink`` are removed, the ``Sink`` instances are destroyed, and any associated files are closed automatically.

Logger Creation
---------------

.. code:: cpp

     auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

     quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

     LOG_INFO(logger, "Hello from {}", "library foo");

Logger Retrieval
----------------

.. code:: cpp

    quill::Logger* logger = quill::Frontend::get_logger("root");

Simplifying Logger Usage for Single Root Logger Applications
------------------------------------------------------------

For some applications the use of the single root logger might be enough. In that case passing the logger everytime
to the macro becomes inconvenient. The solution is to store the created ``Logger`` as a static variable and create your
own macros. See `example <https://github.com/odygrd/quill/blob/master/examples/recommended_usage/quill_wrapper/include/quill_wrapper/overwrite_macros.h>`_
