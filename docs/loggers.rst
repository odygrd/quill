.. title:: Loggers

Loggers
=======

``Logger`` instances are created by users with a specified name, along with designated Sinks and a Formatter. Direct instantiation of logger objects is not possible; instead, they must first be created with the desired configurations.

Upon creation, users specify the Sinks and Formatter for the logger. The logger name serves as an identifier, allowing the same logger configuration to be reused without re-specifying Sinks and Formatter settings.

The Logger class is thread-safe.

.. note::
   Due to the asynchronous design of the library, logger parameters are immutable after creation.
   To modify a logger (such as adding or removing Sinks) you must remove and recreate it.

Logger Creation
---------------

.. code:: cpp

     auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

     quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

     LOG_INFO(logger, "Hello from {}", "library foo");

Logger Retrieval
----------------

Loggers can be obtained using either :cpp:func:`FrontendImpl::get_logger` or :cpp:func:`FrontendImpl::create_or_get_logger`.
The latter will create a new logger if one does not already exist; otherwise, it returns the existing logger with the specified name.

.. code:: cpp

    quill::Logger* logger = quill::Frontend::get_logger("root");

Logger Removal
--------------

Loggers can be removed using :cpp:func:`FrontendImpl::remove_logger`, which deletes them asynchronously. Once this function is called, the logger becomes invalid and should no longer be used for logging. The backend ensures that all pending log statements from the logger are processed before final removal. After removal, a logger with the same name can be recreated with different parameters.

When all loggers associated with a particular ``Sink`` are removed, the ``Sink`` instances are destroyed, and any associated files are closed automatically.

In most cases, creating a new logger is sufficient rather than removing the old one. However, removing a logger can be useful when the underlying sinks need to be destructed and files closed.

For example, if a logger is created for each connected TCP session on a server, with logs written to a separate file, removing the logger upon session disconnection ensures that the underlying file is closed, provided no other logger is sharing the same ``Sink``.

It is possible to explicitly sync :cpp:func:`FrontendImpl::remove_logger`, provided that no other thread attempts to add or remove a logger during the process.

.. literalinclude:: examples/quill_docs_example_loggers_remove.cpp
   :language: cpp
   :linenos:

Simplifying Logger Usage for Single Root Logger Applications
------------------------------------------------------------

For some applications the use of the single root logger might be enough. In that case passing the logger everytime
to the macro becomes inconvenient. The solution is to store the created ``Logger`` as a static variable and create your
own macros. See `example <https://github.com/odygrd/quill/blob/master/examples/recommended_usage/quill_wrapper/include/quill_wrapper/overwrite_macros.h>`_
