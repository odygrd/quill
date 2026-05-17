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

.. note::

   When ``wait_for_queues_to_empty_before_exit`` is enabled, ``Backend::stop()`` assumes frontend threads have stopped logging.
   A few trailing log statements may still drain successfully, but you should not rely on that behavior.
   Sustained concurrent logging during shutdown, especially logging in a loop from another thread, can prevent shutdown from completing because the frontend queues may never become empty.

.. note::

   ``error_notifier``, backend poll hooks, sink periodic tasks, and custom sink ``write_log()`` implementations all run on the backend thread.
   Avoid long-blocking work in these paths, and do not call ``logger->flush_log()`` from them because that would wait on the backend thread itself.

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
