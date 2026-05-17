.. title:: Manual Backend Worker

Manual Backend Worker
=====================

``ManualBackendWorker`` is an advanced integration path for applications that need to run Quill's backend worker on a user-managed thread instead of using :cpp:func:`Backend::start()`.

In most applications you should still prefer the normal backend thread created by :cpp:func:`Backend::start()`. Use ``ManualBackendWorker`` only when you need explicit control over the backend thread's lifecycle or polling loop.

``ManualBackendWorker`` can also be used to run Quill without spawning any additional thread. Applications that already have an event loop or a policy against extra threads can drive the backend by calling ``poll_one()`` from their own thread at a cadence they choose. ``init()`` forces ``sleep_duration = 0`` and ``enable_yield_when_idle = false``, so ``poll_one()`` never sleeps or yields the calling thread — it only does work when there is work to do and returns immediately otherwise.

Note that the frontend hot path is designed for a producer and consumer on different threads. Running both on the same thread still works, but you pay for synchronization and cache-line padding you do not need. Prefer ``Backend::start()`` if it fits your threading model.

Basic Usage
-----------

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

- Use exactly one thread to own and drive the ``ManualBackendWorker``.
- The thread that calls ``init()`` must also call ``shutdown()`` before it exits.
- Do not rely on the destructor for shutdown ordering.
- The manual backend thread may log, but it must not use paths that wait for the backend to flush its own queue. In particular, avoid ``logger->flush_log()`` and ``Frontend::remove_logger_blocking()`` on that same thread. If a logger has immediate flush enabled, the implicit flush is skipped for log calls from the manual backend thread.
- ``Backend::acquire_manual_backend_worker()`` is mutually exclusive with :cpp:func:`Backend::start()`. You can only choose one model per process.
- The built-in signal handler setup is not performed for ``ManualBackendWorker``.

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
