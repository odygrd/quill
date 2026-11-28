.. title:: Backend Options

Backend Options
===============

Pass :cpp:struct:`BackendOptions` to ``Backend::start()`` to tune the worker at runtime. The defaults suit
general use; the linked reference describes each option. Adjust affinity, idle waits, shutdown or character
handling for your workload.

Example Usage
-------------

Pin the worker with ``cpu_affinity``.

.. literalinclude:: snippets/quill_docs_example_backend_options.cpp
   :language: cpp
   :linenos:

CPU Affinity
------------

- Intel macOS uses only the first entry because it lacks per-core affinity masks. Leave ``cpu_affinity``
  empty on Apple Silicon, which lacks this affinity policy.
- Windows CPU IDs are relative to the current processor group. For group-aware placement on machines with
  more than 64 logical processors, leave ``cpu_affinity`` empty and set affinity from a backend-thread hook.
- Affinity failures go to ``error_notifier``. In ``QUILL_NO_EXCEPTIONS`` builds they invoke the fatal error path.

Idle Waits
----------

An idle worker sleeps for :cpp:member:`BackendOptions::sleep_duration` before polling (10 ms on Android,
500 µs elsewhere). Logging leaves it asleep. Any thread may call ``quill::Backend::notify()`` after logging,
even after every statement, at the cost of a condition-variable notification. Pair explicit notifications
with a long sleep for infrequent logs.

With zero sleep the default idle policy busy-waits on a core, using the CPU pause instruction between
empty polls for :cpp:member:`BackendOptions::spin_wait_duration` (default 10 µs). This reduces contention
for producers' queue cache lines. ``enable_yield_when_idle`` can yield instead.

Shutdown
--------

Stop producers before calling ``Backend::stop()`` with ``wait_for_queues_to_empty_before_exit`` enabled (the default).
A few trailing records can drain, but continuous logging can keep shutdown waiting indefinitely. Disabling
the option leaves queued and cached records for a later ``Backend::start()``. Process exit discards them.

With draining enabled, shutdown also drains diagnostics from final-flush and logger-removal close callbacks,
then flushes the sinks again. Failures on this extra pass go directly to stderr to prevent a callback loop.

Backend Callbacks
-----------------

``error_notifier``, poll hooks, sink periodic tasks and ``write_log()`` run on the worker. Keep them short.
If a sink throws, the worker reports it to ``error_notifier`` and delivers the event to later sinks.

Calling ``logger->flush_log()``, ``Backend::stop()`` or ``Frontend::remove_logger_blocking()`` from the
worker throws ``QuillError`` because it would wait for itself. Logging is allowed. A logger with immediate
flush still enqueues the record and skips the implicit wait.

Character Sanitization
-----------------------

By default, sinks receive printable ASCII (space through ``~``), newline (``\n``), tab (``\t``) and carriage
return (``\r``). Other bytes are hex-escaped, including UTF-8 (for example, ``中`` becomes ``\xE4\xB8\xAD``).
To preserve Chinese, Japanese or other non-ASCII text, disable the check as follows.

.. code-block:: cpp

   quill::BackendOptions backend_options;
   backend_options.check_printable_char = {};  // Disable sanitization
   quill::Backend::start(backend_options);

**Custom character filtering.** The backend calls ``check_printable_char`` once for each of the 256 byte
values at startup and caches the answers. The predicate must depend only on its input byte. With exceptions
enabled, a thrown exception marks that byte non-printable.

.. code-block:: cpp

   quill::BackendOptions backend_options;
   backend_options.check_printable_char = [](char c) {
       // Allow ASCII printable + newline + common UTF-8 byte ranges
       return (c >= ' ' && c <= '~') || (c == '\n') || (static_cast<unsigned char>(c) >= 128);
   };
   quill::Backend::start(backend_options);
