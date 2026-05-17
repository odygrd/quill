.. title:: Overview

.. _overview:

Overview
========

The library adopts asynchronous logging to optimise performance, particularly well-suited for low-latency applications where minimizing hot path latency is crucial, such as trading systems.

A dedicated backend thread manages log formatting and I/O operations, ensuring that even occasional log statements incur minimal overhead.

At a glance:

- **Frontend:** captures metadata and a binary copy of the log arguments on the calling thread.
- **Backend:** formats and writes log messages on a dedicated worker thread.
- **Ordering:** a single worker can preserve timestamp order across active queues.

.. image:: design.drawio.svg
   :alt: Quill design overview
   :width: 95%

Thread Safety
-------------

:cpp:type:`Logger` is thread safe by default. The same instance can be used to log by any thread.
Any thread can safely modify the active log level of the logger.

Logging types
-------------

For primitive types, ``std::string``, and ``std::string_view``, the library will perform a deep copy, and all formatting will occur asynchronously in the backend thread.
For standard library types you need to include the relevant file under the ``quill/std`` folder.
For user-defined types you can provide a ``Codec`` specialization to serialize the type or alternatively convert the type to a string on the hot path for non-latency-sensitive code.

See :doc:`Recipes <recipes>` for examples of logging STL types and user-defined types.

Reliable Logging Mechanism
--------------------------

Quill utilizes a thread-local single-producer-single-consumer queue to relay logs to the backend thread.
By default, it uses an unbounded blocking queue with a small initial size to optimise performance.
However, if the queue reaches its capacity, a new queue will be allocated, which may cause a slight performance penalty for the frontend.

The default unbounded queue can expand up to a size of `FrontendOptions::unbounded_queue_max_capacity`. If this limit is reached, the caller thread will block instead of intentionally dropping messages.
It's possible to change the queue type within the :cpp:class:`FrontendOptions`.

The queue size and type are configurable by providing a custom :cpp:class:`FrontendOptions` class.
See :doc:`Frontend Options <frontend_options>` for details.

Manual Log Flushing
-------------------

You can explicitly instruct the frontend thread to wait until all log entries up to the current timestamp are flushed
using :cpp:func:`LoggerImpl::flush_log`. The calling thread will **block** until every log statement up to that point has been flushed.

Synchronized Logs for Debugging
-------------------------------

Sometimes, synchronized logging is necessary during application debugging. This can be achieved by calling :cpp:func:`LoggerImpl::set_immediate_flush` to enable immediate flushing for a specific logger instance.

This causes the calling thread to pause until the log is processed and written to the log file by the backend thread before proceeding, which may have a notable impact on performance.

Note that the immediate flush feature can be completely disabled at compile time by defining `QUILL_ENABLE_IMMEDIATE_FLUSH=0`, which eliminates the conditional branch from the hot path for better performance when immediate flushing is never needed.

Handling Application Crashes
----------------------------

During normal program termination, the library ensures all messages are logged as it goes through the ``BackendWorker`` destructor.

However, in the event of an application crash, some log messages may still be lost.

The library provides an optional built-in signal handler that can help preserve logs in many common crash and termination scenarios. It can be enabled by passing :cpp:struct:`SignalHandlerOptions` to :cpp:func:`Backend::start`.

.. code-block:: cpp

   quill::Backend::start(quill::BackendOptions{}, quill::SignalHandlerOptions{});

On POSIX systems, any thread that may run the built-in handler should either:

- have already logged at least once,
- have called :cpp:func:`FrontendImpl::preallocate`, or
- have the handled signals blocked so the handler does not run on that thread.

If you use your own POSIX signal handler, keep it minimal and avoid calling the general logging or flush APIs from an arbitrary signal context unless you have validated that approach for your platform and process state.

:cpp:struct:`SignalHandlerOptions` allows you to configure:

- ``catchable_signals`` — the list of signals to handle (defaults to ``SIGTERM``, ``SIGINT``, ``SIGABRT``, ``SIGFPE``, ``SIGILL``, ``SIGSEGV``).
- ``timeout_seconds`` — alarm timeout to prevent the process from hanging in the signal handler (Linux only, defaults to 20 seconds).
- ``logger_name`` — the logger to use for crash reporting. If empty, the signal handler automatically selects the first valid logger.
- ``excluded_logger_substrings`` — logger names containing these substrings are skipped during automatic selection (defaults to ``{"__csv__"}``).

On Windows, the handler uses structured exception handling instead of POSIX signals.

Log Messages Timestamp Order
----------------------------

The library employs a single worker backend thread that orders log messages from all queues by timestamp before printing them to the log file.

Number of Backend Threads
-------------------------

Quill prioritizes low latency over high throughput, hence it utilizes only one backend thread to process all logs efficiently. Multiple backend threads are not supported.

Starting and stopping the backend from different threads at the same time is also unsupported. Call
``Backend::start()`` and ``Backend::stop()`` in a coordinated way rather than concurrently.

Latency of the First Log Message
--------------------------------

Upon the first log message from each thread, the library allocates a queue dynamically. For minimizing latency with the initial log, consider calling :cpp:func:`FrontendImpl::preallocate`.

Configuration
-------------

Quill offers various customization options, well-documented for ease of use.

- ``Frontend`` configuration is compile-time, requiring a custom :cpp:class:`FrontendOptions` class.
- For ``Backend`` customization, refer to :cpp:class:`BackendOptions`.

Frontend (caller-thread)
------------------------

The frontend is the calling thread on the user side which issues log statements. It includes:

- **Loggers:** A Logger contains a format pattern and can include one or multiple output Sinks.

- **Sinks:** The Sink serves as the output destination, such as a file, console, or other sources.

Log messages are written using macros that accept a logger as their first argument, followed by a format string. The backend utilizes the ``{fmt}`` library for formatting.

When invoking a ``LOG_`` macro:

1. Creates a static constexpr metadata object to store ``Metadata`` such as the format string and source location.

2. Pushes the data to the SPSC lock-free queue. For each log message, the following variables are pushed:

+------------+---------------------------------------------------------------------------------------------------------------+
| Variable   | Description                                                                                                   |
+============+===============================================================================================================+
| timestamp  | Current timestamp                                                                                             |
+------------+---------------------------------------------------------------------------------------------------------------+
| Metadata*  | Pointer to metadata information                                                                               |
+------------+---------------------------------------------------------------------------------------------------------------+
| Logger*    | Pointer to the logger instance                                                                                |
+------------+---------------------------------------------------------------------------------------------------------------+
| DecodeFunc | A pointer to a templated function containing all the log message argument types, used for decoding the message|
+------------+---------------------------------------------------------------------------------------------------------------+
| Args...    | A serialized binary copy of each log message argument that was passed to the ``LOG_`` macro                   |
+------------+---------------------------------------------------------------------------------------------------------------+

Backend
-------

The backend consists of a single backend thread which takes care of formatting the log statements and the IO writing to files.
Consumes each message from the SPSC queue, retrieves all the necessary information, and then formats the message.
Subsequently, forwards the log message to all ``Sinks`` associated with the Logger.

See Also
--------

- :doc:`Quick Start <quick_start>` for the shortest path to working logs.
- :doc:`Guides <guides>` for sinks, formatters, JSON, filters, and more.
- :doc:`Recipes <recipes>` for common tasks and code examples.
