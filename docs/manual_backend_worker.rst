.. title:: Manual Backend Worker

Manual Backend Worker
=====================

``ManualBackendWorker`` is an advanced integration path for applications that need to run Quill's backend worker on a user-managed thread instead of using :cpp:func:`Backend::start()`.

In most applications you should still prefer the normal backend thread created by :cpp:func:`Backend::start()`. Use ``ManualBackendWorker`` only when you need explicit control over the backend thread's lifecycle or polling loop.

``ManualBackendWorker`` can also be used to run Quill without spawning any additional thread. Applications that already have an event loop or a policy against extra threads can drive the backend by calling ``poll_one()`` from their own thread at a cadence they choose. ``init()`` forces ``sleep_duration = 0`` and ``enable_yield_when_idle = false``, so ``poll_one()`` never sleeps or yields the calling thread — it only does work when there is work to do and returns immediately otherwise.

Note that the frontend hot path is designed for a producer and consumer on different threads. Running both on the same thread still works, but you pay for synchronization and cache-line padding you do not need. Prefer ``Backend::start()`` if it fits your threading model.

Basic Usage
-----------

The default and recommended mode assigns the manual backend worker to one dedicated thread:

1. Start a dedicated thread that will own the manual backend worker.
2. Call :cpp:func:`Backend::acquire_manual_backend_worker()` exactly once for the process.
3. Call ``init()`` on that same thread.
4. Periodically call ``poll()`` or ``poll(timeout)`` on that same thread.
5. Before the thread exits, call ``poll()`` one last time to drain pending log messages, then call ``shutdown()`` on that same thread.

Example
-------

.. literalinclude:: ../examples/manual_backend_worker.cpp
   :language: cpp
   :linenos:

Important Rules
---------------

- Unless thread migration is explicitly enabled, use exactly one thread to own and drive the
  ``ManualBackendWorker``.
- In the default mode, the thread that calls ``init()`` must also call ``shutdown()`` before it
  exits.
- Do not rely on the destructor for shutdown ordering.
- In the default mode, the manual backend thread may log, but it must not use paths that wait for the
  backend to flush its own queue. In particular, avoid ``logger->flush_log()`` and
  ``Frontend::remove_logger_blocking()`` on that same thread. If a logger has immediate flush
  enabled, the implicit flush is skipped for log calls from the manual backend thread.
- ``Backend::acquire_manual_backend_worker()`` is mutually exclusive with :cpp:func:`Backend::start()`. You can only choose one model per process.
- The built-in signal handler setup is not performed for ``ManualBackendWorker``.

Thread Migration
----------------

Applications built around an executor or task pool can allow successive manual backend operations
to run on different threads:

.. code-block:: cpp

   manual_backend_worker->init(backend_options, true);

The executor may use any number of worker threads. A single long-lived coroutine that completes
each polling call before suspending can safely resume on a different thread, provided the executor
synchronizes the continuation handoff. When polling is submitted as separate tasks, bind every task
to the same strand or serial executor; posting independent polling tasks directly to a
multi-threaded executor is unsafe because they can overlap.

This is an advanced mode with reduced safety checks. The caller must satisfy all of the following
requirements:

- Calls to ``poll_one()``, ``poll()``, and ``shutdown()`` must be externally serialized and must
  never overlap.
- Completion of ``init()`` and each worker operation must *happen-before* the next operation starts.
  A mutex, serial executor or strand, or an explicit task dependency normally provides this
  synchronization. Timing alone is not sufficient.
- ``shutdown()`` must run only after every scheduled polling task has completed.
- Hooks, sinks, error notifiers, and other code executed synchronously by the backend must not call
  ``logger->flush_log()``, ``Frontend::remove_logger_blocking()``, or otherwise wait for the backend.
  They must also avoid filling a blocking frontend queue. Quill cannot detect these self-deadlocks
  in thread-migration mode.
- Hooks and custom sinks must tolerate being invoked from different threads over time.
- Quill's signal handler cannot log or flush in thread-migration mode because there is no stable
  backend thread to exclude. The built-in signal handler is not installed for any
  ``ManualBackendWorker`` mode.

Normal producer threads can still call ``flush_log()`` and ``Frontend::remove_logger_blocking()``
provided another task can continue polling the backend. If such a blocking call runs on the same
executor, at least one executor thread must remain available to resume the polling coroutine;
blocking its last available thread will deadlock. ``Backend::get_thread_id()`` returns ``0`` in
thread-migration mode because there is no persistent backend thread identity.

When To Use It
--------------

Good fits:

- integrating Quill into an existing event loop
- running backend polling from a thread already managed by a larger framework
- applications that need strict ownership of backend thread startup and shutdown
- applications with a no-extra-threads policy, where Quill must run without spawning its own backend thread

Avoid it when:

- ``Backend::start()`` already fits your threading model
- you only want a simpler setup path

Driving The Backend From Your Own Thread
----------------------------------------

Call ``poll_one()`` at a natural quiet point of the event loop, often enough to keep frontend queues from filling up.

``poll_one()`` performs formatting and sink I/O on the calling thread. For latency-sensitive threads, a custom sink that forwards records to another process or thread (for example via a shared-memory ring) can keep the polling thread free of heavy I/O. See :doc:`sinks`.

See also :cpp:class:`ManualBackendWorker` and :cpp:func:`Backend::acquire_manual_backend_worker`.
