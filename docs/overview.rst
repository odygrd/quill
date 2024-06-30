.. title:: Overview

Overview
========

The library is split into two parts, the frontend and the backend.

Backend
-------

The backend consists of a single backend thread which takes care of formatting the log statements and the IO writing to files.

Frontend
--------

The frontend is the calling thread on the user side which is issuing the log statements. It consists of ``Loggers`` and ``Sinks``.

- **Loggers:** A Logger contains a format pattern and can contain one or multiple output Sinks.

- **Sinks:** The Sink is the output source, for example a file or console or any other source.

Log messages are written using macros which accept a logger as their first argument,
followed by a format string. The backend is using the {fmt} library for formatting.

Thread Safety
-------------

:cpp:class:`quill::Logger` is thread safe by default. The same instance can be used to log by any thread.
Any thread can safely modify the active log level of the logger.

Logging types
-------------

For primitive types, ``std::string``, and ``std::string_view``, the library will perform a deep copy, and all formatting will occur asynchronously in the backend thread.
For standard library types you need to include the relevant file under the ``quill/std`` folder.
For user-defined types you should provide your own function to serialise the type or alternatively convert the type to string on the hot path for non latency sensitive code
See `user-defined-type-logging-example <https://github.com/odygrd/quill/tree/master/examples/advanced>`_

Guaranteed logging
------------------

Quill employs a thread-local single-producer-single-consumer queue to relay logs to the backend thread,
ensuring that log messages are never dropped.
Initially, an unbounded queue with a small size is used for performance optimization.
However, if the queue reaches full capacity, a new queue will be allocated, incurring a slight performance penalty for the frontend.

The default unbounded queue can expand up to a size of 2GB. Should this limit be reached, the frontend thread will block.
However, it's possible to alter the queue type within the FrontendOptions.

The queue size and type is configurable in runtime by providing custom a :cpp:class:`quill::FrontendOptions` class.

Flush
-----

You can explicitly instruct the frontend thread to wait until the log up to the current timestamp is flushed.

.. note:: The thread that calls :cpp:func:`flush_log` will **block** until every message up to that point is flushed.

.. doxygenfunction:: flush_log

Application Crash Policy
------------------------

When the program is terminated gracefully, quill will go through its destructor where all messages are guaranteed to be logged.

However, if the applications crashes, log messages can be lost.

To avoid losing messages when the application crashes due to a signal interrupt the user must setup a signal
handler and call :cpp:func:`flush_log` inside the signal handler.

There is a built-in signal handler that offers this crash-safe behaviour and can be enabled in :cpp:func:`start_with_signal_handler<quill::FrontendOptions>`

Log Messages Timestamp Order
----------------------------

Quill creates a single worker backend thread which orders the messages in all queues by timestamp before printing them to the log file.

Number of Backend Threads
-------------------------

Quill focus is on low latency and not high throughput. Therefore, there is only one backend thread that will process all logs.

Latency of the first log message
--------------------------------

A queue and an internal buffer will be allocated on the first log message of each thread. If the latency of the first
log message is important it is recommended to call :cpp:func:`quill::preallocate`

.. doxygenfunction:: preallocate()

Configuration
-------------

Quill offers a few customization options, which are also well-documented.

This customization can be applied to either the frontend or the backend.

Frontend configuration occurs at compile time, thus requiring a custom FrontendOptions class to be provided
:cpp:class:`quill::FrontendOptions`

For customizing the backend, refer to :cpp:class:`quill::BackendOptions`