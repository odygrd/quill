.. title:: Backend Options

Backend Options
===============

The backend options allow you to customize the behavior of the backend thread and are applied at runtime. To utilize these options, you need to create an object and pass it when starting the backend thread. Most options come with carefully tuned default values for general use, but they can be adjusted to meet the specific performance and latency requirements of your application. Refer to :cpp:struct:`BackendOptions` for detailed information.

Example Usage
-------------

For example, to pin the backend worker thread to specific CPUs, you can use the following code:

.. literalinclude:: snippets/quill_docs_example_backend_options.cpp
   :language: cpp
   :linenos:

.. note::

   On macOS, only the first CPU entry is used because the platform does not provide a per-core affinity API equivalent to Linux and Windows CPU masks.
   Apple Silicon does not support the affinity policy at all, so leave ``cpu_affinity`` empty there.
   On all platforms, a failure to apply the affinity is reported through ``error_notifier``; in ``QUILL_NO_EXCEPTIONS`` builds it invokes Quill's fatal error path instead.

.. note::

   On Windows, CPU IDs are relative to the backend thread's current processor group. If your application needs group-aware affinity on machines with more than 64 logical processors, leave ``cpu_affinity`` empty and apply affinity yourself from a backend-thread hook.

.. note::

   When ``wait_for_queues_to_empty_before_exit`` is enabled, ``Backend::stop()`` assumes frontend threads have stopped logging.
   A few trailing log statements may still drain successfully, but you should not rely on that behavior.
   Sustained concurrent logging during shutdown, especially logging in a loop from another thread, can prevent shutdown from completing because the frontend queues may never become empty.

.. note::

   ``error_notifier``, backend poll hooks, sink periodic tasks, and custom sink ``write_log()`` implementations all run on the backend thread.
   An exception from one sink is reported through ``error_notifier`` without preventing later sinks from receiving the same event.
   Avoid long-blocking work in these paths. Calling ``logger->flush_log()``, ``Backend::stop()``, or ``Frontend::remove_logger_blocking()`` from these paths throws ``QuillError`` because the backend cannot wait on itself.
   If a logger has immediate flush enabled, backend-thread log calls still enqueue the record, but the implicit flush is silently skipped so generic logging code reused on the backend remains safe.

Character Sanitization
-----------------------

By default, Quill filters log messages to ensure they contain only printable characters before writing them to sinks. This safety feature converts non-printable characters (including non-ASCII characters like Chinese, Japanese, or other Unicode text) to their hexadecimal representation (e.g., ``\xE4\xB8\xAD`` for Chinese characters).

The default printable character range is limited to ASCII characters from space (``' '``) to tilde (``'~'``), plus newline (``'\n'``), tab (``'\t'``), and carriage return (``'\r'``).

**Disabling Character Sanitization for UTF-8 Logging**

If you need to log UTF-8 or other non-ASCII text (such as Chinese, etc.), you can disable character sanitization:

.. code-block:: cpp

   quill::BackendOptions backend_options;
   backend_options.check_printable_char = {};  // Disable sanitization
   quill::Backend::start(backend_options);

**Custom Character Filtering**

You can also define custom printable character rules:

.. code-block:: cpp

   quill::BackendOptions backend_options;
   backend_options.check_printable_char = [](char c) {
       // Allow ASCII printable + newline + common UTF-8 byte ranges
       return (c >= ' ' && c <= '~') || (c == '\n') || (static_cast<unsigned char>(c) >= 128);
   };
   quill::Backend::start(backend_options);
