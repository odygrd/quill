.. _features:

##############################################################################
Features
##############################################################################

Thread Safety
=============

All components and API offered to the user is intended to be thread-safe without any special work needing to be done.

:cpp:class:`quill::LoggerImpl` are thread safe by default. The same instance can be used to log by any thread.
Any thread can safely modify the active log level of the logger.

Logging types
=====================================================

For primitive types, std::string, and std::string_view, the library will perform a deep copy, and all formatting will occur asynchronously in the backend thread.
However, for standard library types or user-defined types, they need to be converted to a string beforehand before being passed to the logger.

Guaranteed logging
=======================

Quill employs a thread-local single-producer-single-consumer queue to relay logs to the backend thread,
ensuring that log messages are never dropped.
Initially, an unbounded queue with a small size is used for performance optimization.
However, if the queue reaches full capacity, a new queue will be allocated, incurring a slight performance penalty for the frontend.

The default unbounded queue can expand up to a size of 2GB. Should this limit be reached, the frontend thread will block.
However, it's possible to alter the queue type within the FrontendOptions.

Customising the queue size and type
--------------------------------------

The queue size and type is configurable in runtime by creating a custom FrontendOptions class.

Flush
===============================

You can explicitly instruct the frontend thread to wait until the log is flushed.

.. note:: The thread that calls :cpp:func:`flush_log` will **block** until every message up to that point is flushed.

.. doxygenfunction:: flush_log

Application Crash Policy
========================

When the program is terminated gracefully, quill will go through its destructor where all messages are guaranteed to be logged.

However, if the applications crashes, log messages can be lost.

To avoid losing messages when the application crashes due to a signal interrupt the user must setup it’s own signal
handler and call :cpp:func:`flush_log` inside the signal handler.

There is a built-in signal handler that offers this crash-safe behaviour and can be enabled in :cpp:func:`start_with_signal_handler<quill::FrontendOptions>`

Log Messages Timestamp Order
==============================

Quill creates a single worker backend thread which orders the messages in all queues by timestamp before printing them to the log file.

Number of Backend Threads
============================

Quill focus is on low latency and not high throughput. Therefore, there is only one backend thread that will process all logs.

Latency of the first log message
====================================

A queue and an internal buffer will be allocated on the first log message of each thread. If the latency of the first
log message is important it is recommended to call :cpp:func:`quill::preallocate`

.. doxygenfunction:: preallocate()

Configuration
======================

Quill offers a few customization options, which are also well-documented.

This customization can be applied to either the frontend or the backend.

Frontend configuration occurs at compile time, thus requiring a custom FrontendOptions class to be provided
:cpp:class:`quill::FrontendOptions`

For customizing the backend, refer to :cpp:class:`quill::BackendOptions`
