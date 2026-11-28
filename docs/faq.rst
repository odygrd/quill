.. title:: Frequently Asked Questions

Frequently Asked Questions (FAQ)
================================

Find answers to setup, performance and integration questions, including missing output, shutdown hangs and
platform-specific issues.

.. contents:: Questions
   :local:
   :depth: 2
   :backlinks: none
   :class: quill-page-index quill-page-index-grouped

Getting Started
---------------

Where to look next?
~~~~~~~~~~~~~~~~~~~

See :ref:`recipes` for examples organized by task.

What is the best way to setup Quill?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For a larger application, build initialization once in a small static library. Follow the
`recommended usage example <https://github.com/odygrd/quill/tree/master/examples/recommended_usage>`_.

Define ``CustomFrontendOptions`` early so you can tune it later. Derive from ``quill::FrontendOptions``,
override only the settings you need and use the same ``CustomFrontend`` and ``CustomLogger`` aliases everywhere.
See `custom_frontend_options.cpp <https://github.com/odygrd/quill/blob/master/examples/custom_frontend_options.cpp>`_.

Can I mix multiple ``FrontendOptions`` specializations in one binary?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

No. Choose one ``FrontendOptions`` specialization per application and use its ``FrontendImpl<T>`` and
``LoggerImpl<T>`` types consistently. Each thread has one frontend context, and the built-in signal handler
assumes one frontend type. See :ref:`frontend_options` for type and configuration checks.

Core Concepts & Behavior
-------------------------

Does Quill offer synchronous/sync mode?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Not natively: formatting and I/O always happen on the backend thread. For debugging,
``logger->set_immediate_flush(1)`` makes the caller wait for each backend write.
``set_immediate_flush(N)`` waits every N messages, preserving some batching. See
:ref:`overview:synchronized logs for debugging` for the cost and compile-time switch.

Does Quill drop log messages?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The default queue grows during bursts and blocks at its limit for records that fit. For dropping modes,
allocation failures and oversized records, see :ref:`frontend_options` and :cpp:class:`FrontendOptions`.

Does Quill support multiple backend threads?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

No. Quill uses one worker per process to keep caller latency low; it merges the records of all threads in
timestamp order. See the :ref:`overview` for how it does so and where throughput is limited.

What's the difference between macro and macro-free mode?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``LOG_INFO(logger, "message {}", arg)``, the recommended default, has the lowest latency and skips the arguments
of disabled levels. ``quill::info(logger, "message {}", arg)`` uses function syntax and always evaluates them; use
it where clarity matters more than speed. See :ref:`macro_free_mode:performance trade-offs` for C++17,
C++20, runtime formats and tags.

What happens if my format string is invalid?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To limit template instantiations and compile time, the backend checks {fmt} strings when it formats them.
In an exception-enabled build, a bad format produces a ``[Could not format log statement ...]`` sink entry
and notifies :cpp:member:`BackendOptions::error_notifier`. The application and backend continue. An empty
notifier (``{}``) suppresses the callback only.

With ``-fno-exceptions`` and ``QUILL_NO_EXCEPTIONS``, a malformed format or argument mismatch calls ``abort()``,
even for string literals. Validate formats before deploying these builds and avoid runtime-supplied formats.

Logging Different Types
------------------------

How do I log my custom types?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Provide formatting (``fmtquill::formatter<T>`` or ``format_as(T)``) and a ``quill::Codec<T>`` specialization.
The :ref:`user-type recipes <recipes:logging user defined types>` cover deferred, direct, custom and
helper-generated codecs.

Can I log STL containers with my custom types?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Yes. Provide the element formatter and codec and include the ``quill/std/`` header for each container type.
See :ref:`recipes:logging nested stl library types`.

Can I use my own libfmt version with Quill?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Quill's bundled {fmt} uses ``fmtquill`` so standalone ``fmt`` can coexist. Share ``format_as(T)`` through
argument-dependent lookup (ADL), or share a helper between separate formatter specializations. See
:ref:`recipes:sharing formatting with external fmt` and ``examples/user_defined_types_logging_deferred_format_as.cpp``.

Performance & Optimization
---------------------------

How can I reduce compile time?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Keep backend and sink headers in the :ref:`setup library <faq:what is the best way to setup quill?>`.
Once initialized, files that log need only ``Logger.h`` and ``LogMacros.h``.

How can I make the binary smaller and use less memory?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For embedded Linux and other size-constrained deployments, apply any of these independently.

**Build settings**

- Compile backend, sink and formatting setup with ``-Os``. Other files can keep their own flags, e.g.
  ``set_source_files_properties(quill_setup.cpp PROPERTIES COMPILE_OPTIONS -Os)``. Shared inline functions
  such as queue growth are kept once and can come from either build. If other files also include
  ``quill/bundled/fmt/format.h`` (e.g. custom formatters), set ``FMTQUILL_USE_FULL_CACHE_DRAGONBOX``
  consistently across files. Its default depends on ``-Os``.
- Define ``FMTQUILL_USE_LOCALE=0`` for every Quill translation unit, e.g. with ``target_compile_definitions``.
  Numeric ``{:L}`` then ignores the application's locale. With static libstdc++, this can save hundreds of KB
  if nothing else uses ``std::locale`` or iostreams, including chrono or ``operator<<`` formatting.
- Define ``QUILL_NO_EXCEPTIONS`` with ``-fno-exceptions`` (MSVC ``/EHs-c- /D_HAS_EXCEPTIONS=0``). An error that
  would throw ``QuillError`` then prints a message and aborts. I/O failures of the file, stream and systemd sinks
  are still reported to ``BackendOptions::error_notifier`` (see :doc:`sinks`).
- Let ELF linkers discard unused code with ``-ffunction-sections -fdata-sections`` and ``-Wl,--gc-sections``.
  On Apple platforms use ``-Wl,-dead_strip``.
- For MSVC, compile setup with ``/O1`` and use ``/GL`` with ``/LTCG``. With ``/DEBUG`` (e.g. ``RelWithDebInfo``),
  pass ``/OPT:REF,ICF`` explicitly to enable those link optimizations.

**Call-site size**

- Set ``QUILL_COMPILE_ACTIVE_LOG_LEVEL`` to remove unused level-specific macros such as ``LOG_INFO``; it does not
  remove ``LOG_DYNAMIC`` or the macro-free functions. See :doc:`log_levels`.
- Each fixed-level statement stores a ``"path:line"`` string and function name. ``QUILL_DISABLE_FILE_INFO`` and
  ``QUILL_DISABLE_FUNCTION_NAME`` omit them. ``-fmacro-prefix-map=<source dir>/=`` shortens paths.
- With many logging sites, try GCC ``-O2`` or ``-Os``. At ``-O3`` GCC can inline hundreds of bytes at each site.
  The other settings more often leave a level check and a shared call for a given pack of argument types.

**Queue and buffer size**

Reduce ``FrontendOptions::initial_queue_capacity`` (default 128 KiB) and use a bounded queue to limit
per-thread memory. It allocates twice its configured capacity. Size it for the largest record, including
its header and arguments such as the full contents of a ``std::string``. See :ref:`frontend_options` for
oversized records, full queues and the backend-thread exception to blocking.

Reduce the backend's transit-event capacity and limits too. ``BackendOptions::transit_events_hard_limit``
counts events per producer, regardless of formatted message size.

.. literalinclude:: snippets/quill_docs_small_footprint.cpp
   :language: cpp
   :linenos:

Is logging from a shared library slower?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On Linux, ``-fPIC`` can add a dynamic TLS lookup to every enabled call (e.g. ``__tls_get_addr`` on x86-64).
``-ftls-model=initial-exec`` avoids it for libraries linked at startup. A library built this way can fail to
load with ``dlopen()``.

With MSVC, startup-linked and ``LoadLibrary()`` DLLs read ``thread_local`` state using the same instructions as
an executable, without a special flag or function call. ``QUILL_DLL_IMPORT`` also preserves static-build
call-site code, entering the other DLL only for slow paths such as first use and flushing.

I am using Quill in tests and each test starts and stops the backend making it very slow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If a test needs no output, skip ``Backend::start()`` and ``Backend::stop()``. Records can still queue, subject
to the :doc:`queue's capacity and policy <frontend_options>`. If a test needs output, choose the system clock
for all its loggers to avoid the backend's lazy first-use :ref:`TSC calibration <timestamp_types:timestamp methods>`,
which takes a few milliseconds.

The backend thread consumes too much CPU
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Check :cpp:member:`BackendOptions::sleep_duration`. Zero with the default idle policy busy-waits on a core.
Increase the sleep duration to reduce CPU use at the cost of write latency. See :ref:`backend_options:idle waits`
for :cpp:member:`BackendOptions::spin_wait_duration` and explicit wake-ups.

Logging drains the battery of a mobile device
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Frequent wake-ups keep a processor out of deep sleep even when average CPU use is low. Log calls never wake the
backend; it wakes after each ``sleep_duration`` (10 ms by default on Android). On mobile, try a sleep
of tens of milliseconds or more if logcat and other sinks can wait that long. For infrequent events, combine
a long sleep with :ref:`explicit notification <backend_options:idle waits>`.

Advanced Configuration
-----------------------

How do I ensure logs aren't lost on application crash?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Enable the built-in handler with ``SignalHandlerOptions`` at startup. Preservation is best effort. Follow
:ref:`overview:handling application crashes` for POSIX per-thread preparation, Windows CRT setup, supported
signals and custom-handler constraints.

How can I use different queue behaviors for production vs simulation/debug?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For distinct policies, select dropping or blocking :ref:`frontend options <frontend_options>` in separate
build configurations. For one build, use a dropping queue and enable ``logger->set_immediate_flush(1000)``
only in simulation. Periodic waits let the backend catch up, but a burst can still overflow between waits.
Choose blocking for records that must be retained.

Can I keep logging while ``Backend::stop()`` is running?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Stop or join producers first. ``Backend::stop()`` normally waits for empty queues, which continued logging can
prevent. :ref:`backend_options:shutdown` covers disabling the drain and restarting. The standard
``Backend::start()`` path registers shutdown at normal process exit. Stop explicitly if you need an earlier,
deterministic shutdown.

Can I log from destructors of static/global objects?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Avoid it. ``LoggerManager``, ``SinkManager`` and ``ThreadContextManager`` are function-local statics. A global
object initialized before those managers can outlive them in reverse destruction order. Logging then accesses
destroyed state. Logging from static constructors and starting the backend before ``main()`` are supported.

Can I log from destructors of thread-local objects?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Only while Quill's context is alive. If a ``thread_local`` object initializes before the thread's first log,
its destructor runs after Quill's context destructor. The backend may already have reclaimed that context.
If destructor logging is unavoidable, initialize Quill on the same thread before first accessing the object.

.. code-block:: cpp

   quill::Frontend::preallocate();
   initialize_or_access_my_thread_local_object();

This orders the object's destruction before Quill's. Keep the backend running and the logger valid. Prefer
explicit cleanup before thread exit.

Can I unload a shared library or plugin that logs with Quill?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Pending records, including backtraces, refer to call-site metadata and decoder code in the library. Complete
cleanup **before** ``dlclose()`` or ``FreeLibrary()``, outside ``DllMain`` and other unload callbacks.

**Library-owned loggers.** Export a cleanup function and call it before unloading. Order matters.

1. Release the library's sink pointers. A sink still held when the logger is removed can stay registered
   even after the library releases it.
2. Call ``Frontend::remove_logger_blocking()`` on each logger the library created while the backend runs.
3. If the library started the backend, call ``Backend::stop()``. Stopping alone preserves logger and sink
   registrations for restart. See ``examples/shared_library``.

**Application-owned logger.** Keep the logger registered. On each library thread, call ``flush_log()`` after
its final library statement. Another thread's flush can miss a record with an equal timestamp. After those
calls return, the unloading thread calls ``flush_backtrace()`` if the logger stores a backtrace, then
``flush_log()``. Unload only after this final flush returns.

Can I use Quill with fork()?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Call ``Backend::start()`` after ``fork()`` in each process that logs. The child does not inherit the running
backend thread. Give parent and child separate files.

.. literalinclude:: snippets/quill_docs_fork.cpp
   :language: cpp
   :linenos:

Troubleshooting
----------------

I see MSVC warning C4275 when exporting a class from a wrapper DLL
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

C4275 can occur when a wrapper DLL exports a class derived from a Quill type without a DLL interface,
such as a custom filter. Create and attach concrete extension classes privately in the DLL that owns setup.
Export small functions or opaque handles. For lookup by logger or sink name, keep a private registry of
wrapper-owned pointers or callbacks.

This keeps C++ layout, STL members, vtables, constructors, destructors and allocator ownership within the DLL.

My log messages are not appearing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Call ``quill::Backend::start()`` early in ``main()`` so the worker can process queued records. ``simple_logger()``
starts it automatically.

I see ``LOG_INFO`` macro conflicts with syslog.h or systemd headers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``<syslog.h>`` and ``<systemd/sd-journal.h>`` define names such as ``LOG_INFO`` and ``LOG_WARNING``. See
:ref:`sink_types:syslogsink` for header isolation and prefixed macros.

Compile error when logging an STL container
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Include the matching codec header, e.g. ``quill/std/Map.h`` for ``std::map``. Nested containers need headers
for every layer. See the :ref:`vector-of-pairs example <recipes:logging nested stl library types>`.

Application hangs on shutdown
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Call ``Frontend::remove_logger_blocking()`` while the worker is running. It can wait indefinitely after
shutdown and cannot run in a backend callback. Stop producers before the worker drains. See
:ref:`loggers:logger removal` and :ref:`backend_options:shutdown`.
