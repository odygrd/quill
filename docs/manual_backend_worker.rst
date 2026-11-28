.. title:: Manual Backend Worker

Manual Backend Worker
=====================

Use :cpp:class:`ManualBackendWorker` when your framework owns the polling thread or event loop. For a
library-managed thread, use :cpp:func:`Backend::start()`.

Basic Usage
-----------

By default, one dedicated thread owns the worker from initialization through shutdown.

1. Start the thread and call :cpp:func:`Backend::acquire_manual_backend_worker()` once for the process.
2. On that thread, call ``init()``, then ``poll()`` or ``poll(timeout)`` periodically.
3. Call ``shutdown()`` on the same thread before it exits. It drains pending logs unless
   ``BackendOptions::wait_for_queues_to_empty_before_exit`` is false.

Example
-------

.. literalinclude:: ../examples/manual_backend_worker.cpp
   :language: cpp
   :linenos:

Important Rules
---------------

- Choose either manual acquisition or ``Backend::start()`` for the process. Shut down explicitly to control
  ordering. Only :ref:`manual_backend_worker:thread migration` permits a change of owner thread.
- In the default mode the owner may log, but cannot wait on itself with ``logger->flush_log()`` or
  ``Frontend::remove_logger_blocking()``. Backend log calls skip implicit immediate flush.
- Quill leaves thread creation, affinity and naming to the application. ``BackendOptions::cpu_affinity``
  and ``BackendOptions::thread_name`` have no effect. Manual workers do not install the built-in signal handler.

Thread Migration
----------------

Enable migration to run successive operations on different threads in an executor or task pool.

.. code-block:: cpp

   manual_backend_worker->init(backend_options, true);

Migration relaxes Quill's thread checks. The application must serialize operations and keep a thread available for polling.

- Serialize ``poll_one()``, ``poll()`` and ``shutdown()``. Completion of ``init()`` and each subsequent operation
  must *happen-before* the next operation starts. Use a mutex, strand, serial executor or explicit task
  dependency. Timing calls apart is insufficient. Shut down after all scheduled polls finish.
- A long-lived coroutine may resume on any executor thread if it completes each poll before suspension and
  the executor synchronizes the handoff. For separate polling tasks, bind all to the same strand or serial
  executor. Independent tasks on a multi-threaded executor can overlap.
- Hooks, sinks and error notifiers must tolerate changing threads. They must avoid waiting on the backend
  (including ``flush_log()`` and ``remove_logger_blocking()``) or filling a blocking frontend queue. Quill
  cannot detect these self-deadlocks in migration mode.
- Producers may call either blocking API if another task can continue polling. On a shared executor, keep
  at least one thread available to resume the poller.

``Backend::get_thread_id()`` returns ``0``. Without a stable backend thread to exclude, Quill's signal handler
cannot log or flush in this mode.

When To Use It
--------------

Manual polling suits a framework-owned thread, an existing event loop, strict startup and shutdown
requirements, or a policy against extra threads. ``Backend::start()`` is simpler when those constraints
do not apply.

Driving The Backend From Your Own Thread
----------------------------------------

Call ``poll_one()`` at a quiet point in your loop, often enough to keep queues from filling. ``init()`` forces
``sleep_duration = 0``, ``enable_yield_when_idle = false`` and ``spin_wait_duration = 0``, so an idle poll
returns without sleeping, yielding or spinning.

Formatting and sink I/O run on the polling thread. A :doc:`custom sink <sinks>` can forward records to
another thread or process, for example through a shared-memory ring. A single thread can produce and
consume, though the frontend still pays for synchronization and cache-line padding designed for separate threads.

See :cpp:func:`Backend::acquire_manual_backend_worker` for acquisition.
