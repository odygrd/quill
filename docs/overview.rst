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

Reliable Logging Mechanism
--------------------------

Quill utilizes a thread-local single-producer-single-consumer queue to relay logs to the backend thread, ensuring that log messages are never dropped.
Initially, an unbounded queue with a small size is used to optimize performance.
However, if the queue reaches its capacity, a new queue will be allocated, which may cause a slight performance penalty for the frontend.

The default unbounded queue can expand up to a size of 2GB. If this limit is reached, the caller thread will block.
It's possible to change the queue type within the :cpp:class:`quill::FrontendOptions`.

The queue size and type are configurable at runtime by providing a custom :cpp:class:`quill::FrontendOptions` class.

Manual Log Flushing
-------------------

You can explicitly instruct the frontend thread to wait until all log entries up to the current timestamp are flushed
using :cpp:func:`quill::LoggerImpl::flush_log`. The calling thread will **block** until every log statement up to that point has been flushed.

Handling Application Crashes
---------------------------

During normal program termination, the library ensures all messages are logged as it goes through the ``BackendWorker`` destructor.

However, in the event of an application crash, some log messages may be lost.

To prevent message loss during crashes caused by signal interrupts, users should set up a signal handler and invoke :cpp:func:`quill::LoggerImpl::flush_log` within it.

The library provides a built-in signal handler that ensures crash-safe behavior, which can be enabled via :cpp:func:`quill::Backend::start_with_signal_handler`.

Log Messages Timestamp Order
----------------------------

The library employs a single worker backend thread that orders log messages from all queues by timestamp before printing them to the log file.

Number of Backend Threads
-------------------------

Quill prioritizes low latency over high throughput, hence it utilizes only one backend thread to process all logs efficiently. Multiple backend threads are not supported.

Latency of the First Log Message
--------------------------------

Upon the first log message from each thread, the library allocates a queue dynamically. For minimizing latency with initial log, consider calling :cpp:func:`quill::FrontendImpl::preallocate`.

Configuration
-------------

Quill offers various customization options, well-documented for ease of use.

- ``Frontend`` configuration is compile-time, requiring a custom :cpp:class:`quill::FrontendOptions` class.
- For ``Backend`` customization, refer to :cpp:class:`quill::BackendOptions`.