.. title:: Loggers

Loggers
=======

``Logger`` instances are created by users with a specified name, along with designated Sinks and a Formatter. Direct instantiation of logger objects is not possible; instead, they must first be created with the desired configurations.

Upon creation, users specify the Sinks and Formatter for the logger. The logger name serves as an identifier, allowing the same logger configuration to be reused without re-specifying Sinks and Formatter settings.

The Logger class is thread-safe.

.. note::
   Due to the asynchronous design of the library, logger parameters are immutable after creation.
   To modify a logger (such as adding or removing sinks), you must remove it, wait for its removal, and then recreate it with the same name.
   Additionally, you should update any stored references to the logger*, as the newly created logger will have a different memory address.


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

Loggers can be removed using :cpp:func:`FrontendImpl::remove_logger` or :cpp:func:`FrontendImpl::remove_logger_blocking`, which deletes them asynchronously. Once this function is called, the logger becomes invalid and should no longer be used for logging by any other thead. The backend ensures that all pending log statements from the logger are processed before final removal. After removal, a logger with the same `logger_name` can be recreated with different parameters.

If you plan to use logger removal functions, ensure that no other threads continue using the logger after calling :cpp:func:`remove_logger` or :cpp:func:`FrontendImpl::remove_logger_blocking`. To prevent issues, consider creating a separate logger for each application thread, each with a unique `logger_name`. This way, when a thread removes its logger, it does not affect others.

When all loggers associated with a particular ``Sink`` are removed, the ``Sink`` instances are destroyed, and any associated files are closed automatically.

In most cases, creating a new logger is sufficient rather than removing the old one. However, removing a logger can be useful when the underlying sinks need to be destructed and files closed.

For example, if a logger is created for each connected TCP session on a server, with logs written to a separate file, removing the logger upon session disconnection ensures that the underlying file is closed, provided no other logger is sharing the same ``Sink``.

.. literalinclude:: examples/quill_docs_example_loggers_remove.cpp
   :language: cpp
   :linenos:

.. literalinclude:: ../examples/logger_removal_with_file_event_notifier.cpp
   :language: cpp
   :linenos:

Simplifying Logger Usage for Single Root Logger Applications
------------------------------------------------------------

For some applications the use of the single root logger might be enough. In that case passing the logger everytime
to the macro becomes inconvenient. The solution is to store the created ``Logger`` as a static variable and create your
own macros. See `example <https://github.com/odygrd/quill/blob/master/examples/recommended_usage/quill_wrapper/include/quill_wrapper/overwrite_macros.h>`_
