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

Loggers can be removed using :cpp:func:`FrontendImpl::remove_logger` or :cpp:func:`FrontendImpl::remove_logger_blocking`. These functions remove the logger asynchronously. Once a logger is removed, it becomes invalid and must no longer be used by any other thread. The backend ensures that all pending log statements from the logger are processed before final removal. After removal, a logger with the same `logger_name` can be recreated with different parameters.

If you plan to use logger removal functions, ensure that no other threads continue using the logger after calling :cpp:func:`remove_logger` or :cpp:func:`FrontendImpl::remove_logger_blocking`. To avoid potential issues, it is recommended to create a separate logger for each application thread, giving each logger a unique `logger_name`. This ensures that when one thread removes its logger, it does not affect other threads.

When all loggers associated with a particular ``Sink`` are removed, the corresponding ``Sink`` instances are destroyed, and any associated files are closed automatically.

In many cases, it is sufficient to create a new logger rather than removing an old one. However, logger removal is particularly useful when the underlying sinks need to be destructed and files closed.

For example, if your server creates a logger for each connected TCP session and writes logs to a separate file for each session, removing the logger upon session disconnection ensures that the underlying file is closed — provided no other logger is sharing the same ``Sink``.

.. note::

   While the logger removal functions (:cpp:func:`FrontendImpl::remove_logger` and :cpp:func:`FrontendImpl::remove_logger_blocking`) are thread-safe, **removing the same logger (`Logger*`) from multiple threads is not allowed**. Ensure that a single thread is responsible for removing a particular logger to avoid undefined behavior.

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
