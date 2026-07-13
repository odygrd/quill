.. title:: Frequently Asked Questions

Frequently Asked Questions (FAQ)
================================

Use this page for setup decisions, common behavior questions, and integration pitfalls.

Getting Started
---------------

Where to look next?
~~~~~~~~~~~~~~~~~~~

Start with :ref:`recipes` for the fastest path to examples and common tasks.

What is the best way to setup Quill?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For non-trivial applications, the recommended setup is to put Quill initialization and setup code in your own small static library that you build once, as shown in the `recommended_usage example <https://github.com/odygrd/quill/tree/master/examples/recommended_usage>`_.

It is also recommended to define your own ``CustomFrontendOptions`` as demonstrated in `custom_frontend_options.cpp <https://github.com/odygrd/quill/blob/master/examples/custom_frontend_options.cpp>`_ and provide the using declarations. Derive from ``quill::FrontendOptions`` and override only the values you want to change. This gives you flexibility to later tune the frontend without copying every field, and you must then consistently use those declarations everywhere else (for example ``CustomFrontend`` and ``CustomLogger``) instead of the library defaults.

Can I mix multiple ``FrontendOptions`` specializations in one binary?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is not a supported configuration.

In normal usage, choose one ``FrontendOptions`` specialization for the application and use the corresponding ``FrontendImpl<T>`` and ``LoggerImpl<T>`` types consistently.

In particular, Quill maintains a single thread-local frontend context per thread, so mixing different frontend specializations on the same thread is unsupported. The built-in signal-handler path also assumes a single frontend specialization for the process.

Core Concepts & Behavior
-------------------------

Does Quill offer synchronous/sync mode?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill does not offer a native synchronous logging mode, as it is designed as an asynchronous logging library where formatting and I/O happen on a dedicated backend thread.

However, you can simulate synchronous behavior using the **immediate flush** feature. By calling ``logger->set_immediate_flush(1)``, the calling thread will block until the log message has been processed and written to its destination by the backend thread. Higher values such as ``logger->set_immediate_flush(N)`` provide periodic flushing every ``N`` messages while preserving some batching.

.. note::

   Using immediate flush impacts performance as it blocks the caller thread. It's primarily intended for debugging scenarios.

See :ref:`overview` for more details.

Does Quill drop log messages?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

No, Quill does not intentionally drop log messages in the default configuration. It uses an **unbounded blocking queue** by default.

The unbounded queue starts with a small initial size and automatically expands when needed (up to ``FrontendOptions::unbounded_queue_max_capacity``). If the maximum capacity is reached, the caller thread will block instead of dropping messages.

You can configure the queue behavior through :cpp:class:`FrontendOptions` to use different queue modes (bounded/unbounded, blocking/dropping) based on your requirements.

See :ref:`frontend_options` for more information on the reliable logging mechanism.

Does Quill support multiple backend threads?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

No, Quill uses a **single backend thread** to process all log messages. This design choice prioritizes low latency over high throughput.

The single backend thread consumes messages from all frontend queues, orders them by timestamp, formats them, and writes them to the configured sinks. This ensures that log messages from multiple threads appear in chronological order in the output.

See :ref:`overview` for details on the backend design.

What's the difference between macro and macro-free mode?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill offers two logging interfaces:

1. **Macro-based logging** (recommended): ``LOG_INFO(logger, "message {}", arg)``
2. **Macro-free logging**: ``quill::info(logger, "message {}", arg)``

The **macro-based approach** is the recommended and default method because it offers lower latency and function arguments are only evaluated when the log level is active.

The **macro-free mode** offers cleaner syntax but comes with performance trade-offs.

For performance-critical code paths, use the macro-based interface. For less critical paths where code clarity is preferred, macro-free mode is acceptable.

What happens if my format string is invalid?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill does not perform ``fmt`` format-string validation at compile time on the logging path. This helps keep compile times and template instantiation overhead lower.

Instead, format strings are validated at runtime when the backend formats the log message.

If the format string is malformed:

- Quill does not crash the backend thread or the application
- Quill writes a fallback ``[Could not format log statement ...]`` entry to the sink output
- if :cpp:member:`BackendOptions::error_notifier` is configured, it is notified too

If ``BackendOptions::error_notifier`` is set to ``{}``, those callback notifications are disabled, but the fallback log entry is still written to the sink output.

Logging Different Types
------------------------

How do I log my custom types?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To log user-defined types, you need to define their formatting and serialization. For formatting, you can either specialize ``fmtquill::formatter<T>`` or provide a free ``format_as(T)`` function. For serialization, provide ``quill::Codec<T>`` or use one of Quill's helper macros/codecs.

Quill provides helper macros and multiple approaches (DeferredFormatCodec, DirectFormatCodec, or custom codecs) to simplify this.

See :ref:`recipes` section on "Logging User Defined Types" for comprehensive examples.

Can I log STL containers with my custom types?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Yes, you can log STL containers containing user-defined types or any type.

Simply define the formatter and codec for your custom type, then include the relevant STL container header from ``quill/std/``.

See :ref:`recipes` for examples of logging nested STL types.

Can I use my own libfmt version with Quill?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill bundles ``libfmt`` under a **custom namespace** (``fmtquill``) to avoid conflicts with external ``libfmt`` installations.

If you're using standalone ``fmt`` in your project too, the preferred way to share formatting customization is a free ``format_as(T)`` function in the same namespace as ``T``. Both ``fmtquill`` and ``fmt`` can find it via ADL, so you avoid duplicate formatter specializations.

If you need a real formatter specialization, provide one ``fmtquill::formatter<T>`` and one ``fmt::formatter<T>`` with a shared helper/base body instead of deriving one from the other.

See :ref:`recipes` section on sharing formatting with external ``fmt`` and the ``examples/user_defined_types_logging_deferred_format_as.cpp`` example for more details.

Performance & Optimization
---------------------------

How can I reduce compile time?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For better compilation time in larger codebases, follow the `recommended_usage example <https://github.com/odygrd/quill/tree/master/examples/recommended_usage>`_.

Build your Quill setup code into a small static library once. Then, after the backend is initialized, most application code only needs the lightweight ``Logger.h`` and ``LogMacros.h`` headers.

I am using Quill in tests and each test starts and stops the backend making it very slow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can try the following to reduce the initialization time in tests:

**Skip starting the backend for most tests**

If a test doesn't actually need logs, avoid calling ``Backend::start()`` and ``Backend::stop()``.
The library is designed to handle this case — your code can still push logs to the queue, but they'll simply never be processed. This avoids the startup cost entirely.

**Use the system clock instead of rdtsc**

If you still need logs in some tests, you can reduce the backend thread initialization time by avoiding ``rdtsc``.
Internally, Quill calibrates ``rdtsc`` lazily on the first ``rdtsc`` timestamp the backend thread sees, which takes a few milliseconds.
Switching all your loggers to use the system clock eliminates that calibration overhead.

The backend thread consumes too much CPU
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default, the backend thread runs in a tight loop polling the queues for new messages, which can consume CPU cycles even when idle.

**Solution 1: Increase the sleep duration**

You can increase the backend sleep duration via :cpp:class:`BackendOptions`. The backend thread will sleep for the specified duration between polling cycles, reducing CPU usage at the cost of slightly increased latency when processing log messages.

**Solution 2: Manual notification**

For applications with infrequent logging, use a very high sleep duration and manually wake up the backend thread when you have messages to process by calling ``quill::Backend::notify()``.

Advanced Configuration
-----------------------

How do I ensure logs aren't lost on application crash?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

During normal program termination, Quill automatically flushes all pending log messages. However, when an application crashes (e.g., segmentation fault, abort), some log messages may be lost if they haven't been processed yet.

Quill provides a built-in signal handler that can preserve pending logs in many common crash and termination scenarios (SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV). To enable it, pass ``SignalHandlerOptions`` when starting the backend with ``quill::Backend::start()``.

On POSIX systems, any thread that may run the built-in handler should either have already logged once, called ``Frontend::preallocate()``, or have the handled signals blocked on that thread.

On Windows, ``Backend::start()`` installs structured exception handling and a console control handler. CRT signal handlers are thread-specific; call ``quill::init_signal_handler<FrontendOptions>()`` on each frontend/user thread that needs CRT signal handling. Do not install the CRT signal handler on the backend worker thread.

**Custom Signal Handler**

If you need to use your own custom signal handler, keep it minimal and avoid calling the general logging or flush APIs from an arbitrary POSIX signal context unless you have validated that approach for your platform and process state.

.. note::

   The built-in signal handler is the practical default when its constraints fit your application, but it should still be treated as a best-effort crash-preservation facility rather than a universal crash-safe logging API.

See :ref:`overview` section on "Handling Application Crashes" for more details.

How can I use different queue behaviors for production vs simulation/debug?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In production, you may want a dropping queue for performance, but in simulation or debug builds, you need to ensure all log messages are captured without drops.

**Solution 1: Use CustomFrontendOptions**

You can switch your build to use ``CustomFrontendOptions`` to configure different queue types for different builds. However, this requires maintaining separate build configurations.

**Solution 2: Single build with runtime control**

A simpler approach is to compile with a dropping queue and control the behavior at runtime. In simulation/debug mode, enable ``logger->set_immediate_flush(1000)``. This will make the caller thread block and wait every 1000 log messages, effectively preventing drops even with a dropping queue. In production, simply don't call ``set_immediate_flush()`` and the queue will drop messages if the backend can't keep up.

Can I keep logging while ``Backend::stop()`` is running?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

No. By default, ``Backend::stop()`` waits for the frontend queues to empty, which assumes frontend threads have already stopped logging.

This behavior is controlled by ``BackendOptions::wait_for_queues_to_empty_before_exit``:

- When it is enabled, a few trailing log statements may still drain successfully, but you should not rely on that behavior. Sustained concurrent logging during shutdown, especially logging in a loop from another thread, can prevent shutdown from completing because the queues may never become empty.
- When it is disabled, the backend may exit earlier, but log messages can be lost during shutdown.

If you call ``Backend::stop()`` explicitly, prefer to stop or join your logging threads first.

In the normal ``Backend::start()`` path, calling ``Backend::stop()`` explicitly is optional because Quill registers automatic shutdown on normal process exit. An explicit call is still useful when you want deterministic shutdown before process exit.

Can I log from destructors of static/global objects?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**No. Avoid logging from the destructor of any global or static-storage-duration object.**

Quill's internal managers (``LoggerManager``, ``SinkManager``, ``ThreadContextManager``) are function-local statics (Meyers singletons). C++ destroys statics in reverse construction order. If your static object's constructor triggers the first log call, the library singletons are constructed *after* your object — and therefore destroyed *before* it. When your object's destructor later attempts to log, it accesses already-destroyed singletons, resulting in undefined behaviour.

Logging from constructors of static objects and calling ``Backend::start()`` before ``main()`` is safe — the singletons will be constructed and remain alive for the lifetime of the program. The problem only arises during shutdown when destruction order is reversed.

Can I log from destructors of thread-local objects?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Only when Quill's thread-local context is still alive. Thread-local objects are destroyed in the
reverse order of their initialization. If a user ``thread_local`` object is initialized before the
first log statement on that thread, Quill's context is initialized later and therefore destroyed
first. Logging from the user's later destructor is then unsupported because that context may
already have been reclaimed by the backend.

If logging from such a destructor is unavoidable, initialize Quill's context before initializing
or first accessing the user thread-local object on the same thread:

.. code-block:: cpp

   quill::Frontend::preallocate();
   initialize_or_access_my_thread_local_object();

This ordering makes the user object's destructor run before Quill's context destructor. The backend
must still be running and the logger must remain valid. Prefer explicit cleanup before thread exit
when possible rather than relying on thread-local destruction order.

Can I use Quill with fork()?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill may not work well with ``fork()`` because it spawns a background thread, and ``fork()`` doesn't work well with multithreading.

If your application uses ``fork()`` and you want to log in the child processes as well, call ``Backend::start()`` **after** the ``fork()`` call. Additionally, write to different files in the parent and child processes to avoid conflicts.

.. code-block:: cpp

   int main()
   {
     // DO NOT call Backend::start() before fork
     if (fork() == 0)
     {
       quill::Backend::start();
       auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("child.log");
       quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));
       LOG_INFO(logger, "Hello from Child {}", 123);
     }
     else
     {
       quill::Backend::start();
       auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("parent.log");
       quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));
       LOG_INFO(logger, "Hello from Parent {}", 123);
     }
   }

Troubleshooting
----------------

I see MSVC warning C4275 when exporting a class from a wrapper DLL
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This can happen when a Windows wrapper DLL exports a C++ class that derives from a Quill type which is not itself exported as a DLL-interface type. Custom filters are one example, but the same DLL-boundary concern can apply to other wrapper-owned extension classes.

Prefer keeping concrete extension classes private to the wrapper implementation. Create and attach them in the same DLL that owns the Quill setup, and export small wrapper functions or opaque wrapper-owned handles for operations the consumer needs.

This avoids exposing C++ class layout, STL members, vtables, constructors/destructors, and allocator ownership across the DLL boundary. For dynamic objects addressed by sink or logger name, the wrapper can keep a private registry from name to a callback or internal pointer owned by the wrapper.

My log messages are not appearing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The most common cause is forgetting to call ``quill::Backend::start()``. Without a running backend thread, log messages remain in the queue and are never processed. Ensure ``Backend::start()`` is called early in ``main()``.

If you are using ``simple_logger()``, the backend is started automatically.

I see ``LOG_INFO`` macro conflicts with syslog.h or systemd headers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Headers like ``<syslog.h>`` and ``<systemd/sd-journal.h>`` define macros named ``LOG_INFO``, ``LOG_WARNING``, etc., which conflict with Quill's ``LOG_`` macros.

**Solution 1**: Include ``SyslogSink.h`` or ``SystemdSink.h`` only in a ``.cpp`` file, not in headers. This confines the collision to a single translation unit.

**Solution 2**: Define ``QUILL_DISABLE_NON_PREFIXED_MACROS`` and use the longer ``QUILL_LOG_INFO`` macros everywhere.

Compile error when logging an STL container
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill does not automatically include codec headers for STL types. To log a ``std::vector``, include ``quill/std/Vector.h``. To log a ``std::map``, include ``quill/std/Map.h``, and so on.

For nested types like ``std::vector<std::pair<int, std::string>>``, include headers for both the outer and inner types (``quill/std/Vector.h`` and ``quill/std/Pair.h``).

Application hangs on shutdown
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If ``Frontend::remove_logger_blocking()`` is called while the backend is not running, it will block indefinitely waiting for the backend to process the removal. Only call this function while the backend is running, and not from backend-thread callbacks.

Calling ``Backend::stop()`` will flush all pending messages and cleanly shut down the backend thread. Ensure all logging is complete before calling ``Backend::stop()``.
