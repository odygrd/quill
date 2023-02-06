.. _features:

##############################################################################
Features
##############################################################################

Thread Safety
=============

All components and API offered to the user is intended to be thread-safe without any special work needing to be done.

:cpp:class:`quill::Logger` are thread safe by default. The same instance can be used to log by any thread.
Any thread can safely modify the active log level of the logger.

Logging a non copyable, non movable user defined type
=====================================================

Quill will copy all arguments passed as arguments and perform all the formatting in the background thread.
Therefore, the arguments passed to the logger needs to be copyable or at least movable.
If the argument can not be made copyable then the user has to convert it to a string first before passing it to the logger.

Guaranteed logging
=======================

Quill uses a thread-local single-producer-single-consumer queue to
forward logs records to the backend thread. By default an unbounded queue is used with an initially small size for
performance reasons. If the queue becomes full the user can suffer a small performance penalty as a new queue will get
allocated. Log messages are never dropped.

Customising the queue size
--------------------------

The queue size if configurable in runtime in :cpp:class:`quill::Config` and applies to both bounded and unbounded queues.

Enabling non-guaranteed logging mode
------------------------------------

If this option is enabled in ``TweakMe.h`` then the queue will never re-allocate but log messages will be dropped instead.
If any messages are dropped then the user is notified by logging the number of dropped messages to ``stderr``

Flush Policy and Force Flushing
===============================

By default quill lets `libc` to flush whenever it sees fit in order to achieve good performance.
You can explicitly instruct the logger to flush all its contents. The logger will in turn flush
all existing handlers.

.. note:: The thread that calls ::cpp:func:`quill::flush()` will **block** until every message up to that point is flushed.

.. doxygenfunction:: quill::flush()

Application Crash Policy
========================

When the program is terminated gracefully, quill will go through its destructor where all messages are guaranteed to be logged.

However, if the applications crashes, log messages can be lost.

To avoid losing messages when the application crashes due to a signal interrupt the user must setup it’s own signal
handler and call :cpp:func:`quill::flush()` inside the signal handler.

There is a built-in signal handler that offers this crash-safe behaviour and can be enabled in :cpp:func:`quill::start()`

Log Messages Timestamp Order
==============================

Quill creates a single worker backend thread which orders the messages in all queues by timestamp before printing
them to the log file.

Number of Backend Threads
============================

Quill focus is on low latency and not high throughput. Therefore, there is only one backend thread that will process all logs.

Latency of the first log message
====================================

A queue and an internal buffer will be allocated on the first log message of each thread. If the latency of the first
log message is important it is recommended to call :cpp:func:`quill::preallocate()`

.. doxygenfunction:: quill::preallocate()

Configuration
======================

Quill offers a few customisation options which are also very well documented.

Have a look at files ``Config.h`` under the namespace :cpp:func:`quill::config`.

Ideally each hot thread runs on an isolated CPU. Then the backend
logging thread should also be pinned to an either isolated or a junk CPU core.

Also the file ``TweakMe.h`` offers some compile time customisations.
In release builds lower severity log levels such as ``LOG_TRACE`` or ``LOG_DEBUG`` statements can be
compiled out to reduce the number of branches in the application. This can be done by editing ``TweakMe.h`` or invoking cmake

::

   cmake .. -DCMAKE_CXX_FLAGS="-DQUILL_ACTIVE_LOG_LEVEL=QUILL_LOG_LEVEL_INFO"