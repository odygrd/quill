.. title:: Sink Types

Sink Types
==========

Reference for all built-in sink implementations. See :doc:`Sinks <sinks>` for how to create, share, and extend sinks.

ConsoleSink
~~~~~~~~~~~

The `ConsoleSink` class sends logging output to streams `stdout` or `stderr`. Printing color codes to terminal or Windows console is also supported.

FileSink
~~~~~~~~

The :cpp:class:`FileSink` is a straightforward sink that outputs to a file. The filepath of the `FileSink` serves as a unique identifier, allowing you to retrieve the same sink later using :cpp:func:`FrontendImpl::get_sink`.

Each file can only have a single instance of `FileSink`.

.. note::

   When using :cpp:class:`FileEventNotifier` with :cpp:class:`FileSink`, the callback handle type is
   platform-dependent and is exposed as ``quill::FileEventNotifierHandle``. On Windows it is a native
   ``HANDLE``. On other platforms it is a ``FILE*``.

   ``before_write`` runs as part of normal log writing. ``before_open``, ``after_open``,
   ``before_close``, and ``after_close`` run on whichever thread performs the file open/close
   operation. Different callbacks, and different invocations of the same callback, may therefore
   run on different threads over the sink lifetime. Callbacks must be thread-safe and must not
   assume a single calling thread. When ``before_write`` changes the message size, size-based
   rotation evaluates its threshold using the pre-transform size of the current message, so a
   rotation can trigger up to one transformed message later or earlier than the exact limit.

.. literalinclude:: snippets/quill_docs_example_file.cpp
   :language: cpp
   :linenos:

RotatingFileSink
~~~~~~~~~~~~~~~~

The :cpp:type:`RotatingFileSink` is built on top of the `FileSink` and provides log file rotation based on specified time intervals, file sizes, or daily schedules.

.. note::

   When the sink starts in append mode, rotated files left behind by previous runs are detected
   for every ``RotationNamingScheme`` and count towards ``max_backup_files`` whether or not the
   base filename has an extension; the oldest ones are removed on startup if they exceed the
   limit, so log files do not accumulate across restarts. Set
   ``set_overwrite_rolled_files(false)`` to prevent append-mode recovery from deleting existing
   rotated files, or use ``FilenameAppendOption::StartDateTime`` to give each run its own
   independent set of files.

   If the startup directory cannot be enumerated safely, existing rotated files are left untouched
   and rotation is disabled for that sink instance. Writes to the active file continue normally.

.. literalinclude:: ../examples/rotating_file_logging.cpp
   :language: cpp
   :linenos:

JsonFileSink/JsonConsoleSink
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. literalinclude:: snippets/quill_docs_example_json_logging.cpp
   :language: cpp
   :linenos:

RotatingJsonFileSink
~~~~~~~~~~~~~~~~~~~~

The :cpp:type:`RotatingJsonFileSink` is built on top of the `JsonFileSink` and provides log file rotation based on specified time intervals, file sizes, or daily schedules.

.. literalinclude:: ../examples/rotating_json_file_logging.cpp
   :language: cpp
   :linenos:

SyslogSink
~~~~~~~~~~

The :cpp:class:`SyslogSink` leverages the syslog API to send messages.

.. note::

   Syslog uses process-global state. Multiple :cpp:class:`SyslogSink` instances are therefore not
   independent if they use different identifiers, options, or facilities. Use a single
   :cpp:class:`SyslogSink` configuration per process.

StreamSink
~~~~~~~~~~

The :cpp:class:`StreamSink` is a base sink class used to write log messages to C-style ``FILE*`` streams, such as 
``stdout``, ``stderr``, or files opened with ``fopen``.
It is typically used as a foundation for higher-level sinks like :cpp:class:`ConsoleSink`.

To wrap an application-owned stream with ``StreamSink``, open it with ``fopen()`` or,
on POSIX, ``fdopen()`` and pass the resulting ``FILE*`` to the sink constructor.
``StreamSink`` does not open the supplied filename or close the stream. The application
must keep the ``FILE*`` alive until the backend has finished using it.

The ``stream_is_unbuffered`` constructor flag is an opt-in for the specific case of an
application-owned, unbuffered stream, such as a pipe writer. It enables retries when a write
is interrupted by a signal. Leave it ``false`` for buffered streams; the flag itself does
not change the stream's buffering mode.

To opt in, call ``setvbuf(stream, nullptr, _IONBF, 0)`` immediately after opening the stream,
before any reads, writes, or logging. Check that it succeeds, then pass the configured
``FILE*`` and ``stream_is_unbuffered=true`` when creating the ``StreamSink``. Keep buffering
disabled for the sink's lifetime.

This complete example uses a regular file so it can run on all supported platforms. The
same setup sequence applies to a POSIX pipe stream opened with ``fdopen()``:

.. literalinclude:: snippets/quill_docs_example_unbuffered_stream.cpp
   :language: cpp
   :linenos:

.. note::

   This opt-in is for direct ``StreamSink`` construction. ``FileSink`` and ``ConsoleSink``
   do not expose it. On POSIX, ``FileSink`` passes its internally opened ``FILE*`` to
   ``FileEventNotifier::after_open``, but changing buffering in that callback does not
   enable the retry flag.

   ``FileSink`` uses a 64 KiB buffer by default. ``FileSinkConfig::set_write_buffer_size(0)``
   selects default buffering; it does not make the stream unbuffered.

Buffered short writes retain the existing error reporting. This feature does not provide
lossless recovery for buffered streams: some C libraries discard pending buffered data on
a write error, and retrying ``fwrite()`` cannot reliably recover the discarded bytes.

Direct callers of the static write helper can use ``StreamSink::safe_fwrite_unbuffered()``
after the same checked ``_IONBF`` setup. ``StreamSink::safe_fwrite()`` retains the default
short-write error handling.

AndroidSink
~~~~~~~~~~~

The :cpp:class:`AndroidSink` sends log messages to the Android logging system using the Android NDK logging API.

NullSink
~~~~~~~~

The :cpp:class:`NullSink` discards all log messages, useful for performance testing or disabling specific logger output without removing logging calls.

.. note:: Macro Collision Notice

   When including ``syslog.h`` via :cpp:class:`SyslogSink`, the header defines macros such as ``LOG_INFO``
   (and others) that may collide with Quill's unprefixed ``LOG_`` macros. To resolve this issue, consider one
   of the following solutions:

   - **Include SyslogSink in a .cpp file only:**
     Instantiate the SyslogSink in a source file rather than in a header file. This ensures that ``syslog.h``
     is included only in that specific translation unit, allowing the rest of your code to use the unprefixed
     Quill ``LOG_`` macros without conflict.

   - **Define the preprocessor flag ``QUILL_DISABLE_NON_PREFIXED_MACROS``:**
     This flag disables the unprefixed Quill ``LOG_`` macros and forces the use of the longer
     ``QUILL_LOG_`` macros instead. This approach allows Quill to work alongside ``syslog.h`` in the same
     translation unit.

   Alternatively, you can combine both solutions if you include :cpp:class:`SyslogSink` in a .cpp file where you
   also want to use the unprefixed ``LOG_`` macros. However, the first solution is generally preferred since it
   allows for less typing with the concise ``LOG_`` macros.

.. code:: cpp

    #define QUILL_DISABLE_NON_PREFIXED_MACROS

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/SyslogSink.h"

    #include <string>
    #include <utility>

    int main()
    {
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

      // Frontend
      auto sink = quill::Frontend::create_or_get_sink<quill::SyslogSink>(
        "app", []()
        {
          quill::SyslogSinkConfig config;
          config.set_identifier("app");
          return config;
        }());

      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(sink));

      QUILL_LOG_INFO(logger, "A {} message with number {}", "log", 1);
      QUILL_LOG_WARNING(logger, "test message {}", 123);
    }

SystemdSink
~~~~~~~~~~~

The :cpp:class:`SystemdSink` integrates with the systemd journal, allowing messages to be sent directly to systemd. To use this sink, ensure the `systemd-devel` package is installed. Additionally, link your program against ``lsystemd``.

.. code:: cmake

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SYSTEMD REQUIRED libsystemd)
    target_link_libraries(${TARGET} ${SYSTEMD_LIBRARIES})

.. note:: Macro Collision Notice

   When including ``syslog.h`` via :cpp:class:`SystemdSink`, the header defines macros such as ``LOG_INFO``
   (and others) that may collide with Quill's unprefixed ``LOG_`` macros. To resolve this issue, consider one
   of the following solutions:

   - **Include SystemdSink in a .cpp file only:**
     Instantiate the SystemdSink in a source file rather than in a header file. This ensures that ``syslog.h``
     is included only in that specific translation unit, allowing the rest of your code to use the unprefixed
     Quill ``LOG_`` macros without conflict.

   - **Define the preprocessor flag ``QUILL_DISABLE_NON_PREFIXED_MACROS``:**
     This flag disables the unprefixed Quill ``LOG_`` macros and forces the use of the longer
     ``QUILL_LOG_`` macros instead. This approach allows Quill to work alongside ``syslog.h`` in the same
     translation unit.

   Alternatively, you can combine both solutions if you include :cpp:class:`SystemdSink` in a .cpp file where you
   also want to use the unprefixed ``LOG_`` macros. However, the first solution is generally preferred since it
   allows for less typing with the concise ``LOG_`` macros.

.. code:: cpp

    #define QUILL_DISABLE_NON_PREFIXED_MACROS

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/SystemdSink.h"

    #include <string>
    #include <utility>

    int main()
    {
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

      // Frontend
      auto sink = quill::Frontend::create_or_get_sink<quill::SystemdSink>(
        "app", []()
        {
          quill::SystemdSinkConfig config;
          config.set_identifier("app");
          return config;
        }());

      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(sink));

      QUILL_LOG_INFO(logger, "A {} message with number {}", "log", 1);
      QUILL_LOG_WARNING(logger, "test message {}", 123);
    }
