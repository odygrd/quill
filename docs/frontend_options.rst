.. title:: Frontend Options

.. _frontend_options:

Frontend Options
================

Derive from :cpp:struct:`FrontendOptions` to choose the queue type, capacity and memory policy at compile
time, including huge pages on Linux. Each logging thread has a queue. Choose one configuration for the application.

- **UnboundedBlocking** (default) grows when full. At ``unbounded_queue_max_capacity`` or on allocation
  failure, the caller waits for space.
- **UnboundedDropping** grows the same way. When full and unable to grow, it drops new records.
- **BoundedBlocking** has fixed capacity and waits for space. A record larger than the queue throws
  ``QuillError`` immediately. A backend callback cannot wait for itself and drops a record if the queue fills.
- **BoundedDropping** has fixed capacity and drops records that do not fit, even an oversized record
  submitted to an empty queue.

``initial_queue_capacity`` defaults to 128 KiB. The unbounded maximum defaults to 2 GiB and must be at
least the initial capacity. Both round up to powers of two. A record larger than the configured unbounded
maximum throws ``QuillError``.

If an unbounded queue cannot grow and an individual record exceeds its current capacity, both modes drop
the record. A dropped MDC update leaves the previous context active. The backend reports these drops separately.
``Logger::flush_log()`` and ``Frontend::remove_logger_blocking()`` throw ``QuillError`` if this prevents enqueueing
their requests.

Override only the values you need. Types without a ``FrontendOptions`` base fail to compile. Use the derived
type consistently with :cpp:class:`FrontendImpl` and :cpp:class:`LoggerImpl` throughout the application.

.. warning::

   Mixing specializations is unsupported. Creating a logger with option values that differ from existing
   loggers throws ``QuillError``. The same check applies to ``Backend::start<TFrontendOptions>()`` with
   ``SignalHandlerOptions`` and, on Windows, each ``quill::init_signal_handler<TFrontendOptions>()`` call.
   The handler logs through that frontend type.

Error Notifications
-------------------

The worker calls :cpp:member:`BackendOptions::error_notifier` for unbounded queue allocations and when
maximum capacity or allocation failure leads to blocking or drops. For full bounded queues it reports the
number of dropped messages.

Queue Memory Management
-----------------------

**Queue shrinking.** After a burst, call :cpp:func:`Frontend::shrink_thread_local_queue` on the thread whose
unbounded queue should release storage. This is useful between tasks in a thread pool.

.. code-block:: cpp

    // After a logging-intensive task completes, shrink the queue
    quill::Frontend::shrink_thread_local_queue(512 * 1024);  // Shrink to 512KB

**Allocation notices.** By default the notifier writes informational queue growth messages to stderr.

.. code-block:: shell

    Allocated a new SPSC queue with a capacity of 256 KiB (previously 128 KiB) from thread 3764

Install a custom notifier to handle these messages differently.

.. code-block:: cpp

    quill::BackendOptions backend_options;
    backend_options.error_notifier = [](const std::string& error_message) {
        // Custom handling - log to your preferred destination
        my_logger.info("Quill: {}", error_message);
    };

An empty function disables callback notifications.

.. code-block:: cpp

    quill::BackendOptions backend_options;
    backend_options.error_notifier = {};  // Disable notifications

With exceptions enabled, a malformed format string still writes ``[Could not format log statement ...]`` to
the sink. Exception-free builds abort. See :ref:`faq:what happens if my format string is invalid?`.

**Hot and cold threads.** Choose a large initial capacity for latency-critical threads, then shrink queues
on threads that log less often. Call ``shrink_thread_local_queue()`` through the ``FrontendImpl`` of your
options, as every other frontend call.

.. literalinclude:: snippets/quill_docs_hot_cold_threads.cpp
   :language: cpp
   :linenos:

This reserves space for expected bursts while limiting memory use on quieter threads.

Example Usage
-------------

This example selects ``BoundedDropping`` and inherits the remaining defaults.

.. literalinclude:: ../examples/custom_frontend_options.cpp
   :language: cpp
   :linenos:
