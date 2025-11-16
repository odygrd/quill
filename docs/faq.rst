.. title:: Frequently Asked Questions

Frequently Asked Questions (FAQ)
================================

Getting Started
---------------

Where to look next?
~~~~~~~~~~~~~~~~~~~

The best page to get information quickly is the :ref:`cheat_sheet` which provides a very detailed how-to guide.

What is the best way to setup Quill?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For flexibility, the recommended way is to wrap Quill's ``Backend`` code in a static library that you build once, as shown in the `recommended_usage example <https://github.com/odygrd/quill/tree/master/examples/recommended_usage>`_.

It is also highly recommended to define your own ``CustomFrontendOptions`` as demonstrated in `custom_frontend_options.cpp <https://github.com/odygrd/quill/blob/master/examples/custom_frontend_options.cpp>`_ and provide the using declarations. This gives you flexibility to later tune the Frontend. You must consistently use the using declarations everywhere else (e.g., ``CustomFrontend``, ``CustomLogger``) instead of the library defaults.

Core Concepts & Behavior
-------------------------

Does Quill offer synchronous/sync mode?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill does not offer a native synchronous logging mode, as it is designed as an asynchronous logging library where formatting and I/O happen on a dedicated backend thread.

However, you can simulate synchronous behavior using the **immediate flush** feature. By calling ``logger->set_immediate_flush(1)``, the calling thread will block until the log message has been processed and written to its destination by the backend thread. This effectively provides synchronous-like behavior.

.. note::

   Using immediate flush impacts performance as it blocks the caller thread. It's primarily intended for debugging scenarios.

See :ref:`overview` for more details.

Does Quill drop log messages?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

No, Quill does not drop log messages by default. It uses an **unbounded queue** with reliable message delivery guarantees.

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

Logging Different Types
------------------------

How do I log my custom types?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To log user-defined types, you need to specialize ``fmtquill::formatter<T>`` for formatting and ``quill::Codec<T>`` for serialization.

Quill provides helper macros and multiple approaches (DeferredFormatCodec, DirectFormatCodec, or custom codecs) to simplify this.

See :ref:`cheat_sheet` section on "Logging User Defined Types" for comprehensive examples.

Can I log STL containers with my custom types?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Yes, you can log STL containers containing user-defined types or any type.

Simply define the formatter and codec for your custom type, then include the relevant STL container header from ``quill/std/``.

See :ref:`cheat_sheet` for examples of logging nested STL types.

Can I use my own libfmt version with Quill?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill bundles ``libfmt`` under a **custom namespace** (``fmtquill``) to avoid conflicts with external ``libfmt`` installations. All formatter specializations for user-defined types must be defined under the ``fmtquill`` namespace.

If you're using an external ``libfmt`` library in your project, you can reuse existing ``fmt::formatter`` specializations by deriving from them. However, you must:

1. **Template both parse() and format()** functions to support different context types
2. **Ensure ABI compatibility**: The major version of your external ``libfmt`` must match Quill's bundled version to avoid ABI incompatibilities and potential runtime errors

See :ref:`cheat_sheet` section on "Using External fmt Formatter Specializations" for more details.

Performance & Optimization
---------------------------

How can I reduce compile time?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For better compilation time, consider following the `recommended_usage example <https://github.com/odygrd/quill/tree/master/examples/recommended_usage>`_.

Wrap and build the Backend part of the library as a static library. Then for logging, once the backend is initialized, you only need to include two headers: ``Logger.h`` and ``LogMacros.h``.

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

Quill provides a built-in signal handler that automatically flushes all pending logs when the application receives crash signals (SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV). To enable it, pass ``SignalHandlerOptions`` when starting the backend with ``quill::Backend::start()``.

**Custom Signal Handler**

If you need to use your own custom signal handler, you can manually call ``logger->flush_log()`` within your handler to ensure all messages up to that point are flushed.

.. note::

   The built-in signal handler is the recommended approach for most use cases as it handles all common crash scenarios automatically.

See :ref:`overview` section on "Handling Application Crashes" for more details.

How can I use different queue behaviors for production vs simulation/debug?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In production, you may want a dropping queue for performance, but in simulation or debug builds, you need to ensure all log messages are captured without drops.

**Solution 1: Use CustomFrontendOptions**

You can switch your build to use ``CustomFrontendOptions`` to configure different queue types for different builds. However, this requires maintaining separate build configurations.

**Solution 2: Single build with runtime control**

A simpler approach is to compile with a dropping queue and control the behavior at runtime. In simulation/debug mode, enable ``logger->set_immediate_flush(1000)``. This will make the caller thread block and wait every 1000 log messages, effectively preventing drops even with a dropping queue. In production, simply don't call ``set_immediate_flush()`` and the queue will drop messages if the backend can't keep up.

Can I use Quill with fork()?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill may not work well with ``fork()`` because it spawns a background thread, and ``fork()`` doesn't work well with multithreading.
See the Caveats section in the `README` for more information.
