.. title:: Sink Types

Sink Types
==========

Choose a built-in sink for your output destination. See :doc:`sinks` for creating, sharing and extending sinks.

ConsoleSink
~~~~~~~~~~~

``ConsoleSink`` writes to ``stdout`` or ``stderr``. ``ConsoleSinkConfig::ColourMode::Automatic`` enables colour
on supported terminals, including Windows consoles, unless ``NO_COLOR`` has a value.

For UTF-8 on a Windows console, call ``SetConsoleOutputCP(CP_UTF8)`` and disable or customize
:cpp:member:`BackendOptions::check_printable_char` to preserve the bytes. Redirection to a pipe or file
writes the bytes without console code-page conversion.

FileSink
~~~~~~~~

:cpp:class:`FileSink` opens one sink per normalized path. Retrieve it by path with :cpp:func:`FrontendImpl::get_sink`.
``FileSinkConfig::set_file_header()`` supplies bytes, such as a CSV header, for every empty file opened,
including rotations. Appending to a non-empty file preserves its header. ``FileEventNotifier::after_open``
sees the file before the header is written.

**Buffered writes.** The sink writes when its own buffer fills or the worker flushes it.
``FileSinkConfig::set_write_buffer_size()`` sets capacity (default 64 KiB, or 256 KiB on Windows).
Writes through the callback handle stay in order with records. ``setvbuf`` on that handle does not resize
the sink buffer. On failure such as a full disk, the sink retains buffered bytes and the remainder of a
record larger than its buffer for a later successful write. Bytes it cannot retain are lost, possibly
leaving a partial record.

**Direct writes.** ``set_write_buffer_size(0)`` bypasses the sink buffer. Each record uses the handle
available to ``after_open`` and ``before_close``. Callbacks can retain and configure it. Direct writes are
slower. Windows writes each record immediately. Elsewhere the ``FILE*`` uses C library buffering unless
binary framing other than ``BinaryRecordFraming::None`` or ``PcapWriter`` selects unbuffered mode.
``after_open`` can change it with ``setvbuf``. On failure an unbuffered handle lets the sink retain the
record remainder. A buffered stream can discard bytes.

**Windows paths.** A ``std::string`` filename uses the active code page. For other characters pass a
``std::filesystem::path`` built from a wide string.

.. code-block:: cpp

    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
      std::filesystem::path{L"C:\\logs\\\u65e5\u8a8c.log"}, quill::FileSinkConfig{});

File Callbacks
^^^^^^^^^^^^^^

:cpp:class:`FileEventNotifier` exposes ``quill::FileEventNotifierHandle`` (Windows
``HANDLE``, otherwise ``FILE*``). ``before_write`` runs during normal writes. ``before_open``, ``after_open``,
``before_close`` and ``after_close`` run on the thread opening or closing the file. Even successive invocations
of the same callback can use different threads, so callbacks must be thread-safe.

If ``before_write`` changes the record's length, size-based rotation still tests its original size. Rotation
can occur up to one transformed message earlier or later than the exact threshold.

.. literalinclude:: snippets/quill_docs_example_file.cpp
   :language: cpp
   :linenos:

RotatingFileSink
~~~~~~~~~~~~~~~~

:cpp:type:`RotatingFileSink` adds size, interval or daily rotation to ``FileSink``.

- In append mode, startup detects previous rotations for every ``RotationNamingScheme``, including base names
  without extensions. All count toward ``max_backup_files`` and startup removes the oldest excess backups.
  ``set_overwrite_rolled_files(false)`` preserves existing files. ``FilenameAppendOption::StartDateTime`` gives
  each run a separate set.
- If cleanup or recovery requires a scan and the directory cannot be listed (even if writable), the sink
  throws ``QuillError`` and leaves the files intact to avoid overwriting an undiscovered backup. Open mode
  ``w`` with ``set_remove_old_files(false)`` skips the scan.

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

:cpp:type:`RotatingJsonFileSink` adds size, interval or daily rotation to ``JsonFileSink``.

.. literalinclude:: ../examples/rotating_json_file_logging.cpp
   :language: cpp
   :linenos:

SyslogSink
~~~~~~~~~~

:cpp:class:`SyslogSink` writes through the process-global syslog API. Use one identifier, option set and
facility per process. Separate instances cannot configure these independently.

**Macro names.** ``<syslog.h>`` and ``<systemd/sd-journal.h>`` define names such as ``LOG_INFO`` and
``LOG_WARNING``. Keep ``SyslogSink.h`` or ``SystemdSink.h`` in the setup ``.cpp`` to confine the conflict.
Other files can then use short ``LOG_*`` names. For logging in the same translation unit, define
``QUILL_DISABLE_NON_PREFIXED_MACROS`` and use ``QUILL_LOG_*``. The two approaches can be combined.

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

StreamSink
~~~~~~~~~~

:cpp:class:`StreamSink` writes to C ``FILE*`` streams, including ``stdout``, ``stderr`` and application files.
It also underlies :cpp:class:`ConsoleSink`. Open your stream with ``fopen()`` or POSIX ``fdopen()`` and pass
the handle to the constructor. The filename is an identifier. The application owns the stream and must
keep it open until the backend finishes.

For an unbuffered stream, such as a pipe writer, enable retries on signal interruption in two steps.

1. Immediately after opening and before any I/O, call ``setvbuf(stream, nullptr, _IONBF, 0)`` and check success.
2. Pass the handle with ``stream_is_unbuffered=true``. The flag enables retries only. Keep buffering disabled
   throughout the sink's lifetime and leave the flag false for buffered streams.

This portable example uses a file. The same sequence works for a POSIX pipe opened with ``fdopen()``.

.. literalinclude:: snippets/quill_docs_example_unbuffered_stream.cpp
   :language: cpp
   :linenos:

The flag is available only on direct ``StreamSink`` construction. ``ConsoleSink`` and ``FileSink`` do not expose
it. On POSIX, changing the ``FILE*`` in ``FileEventNotifier::after_open`` or disabling the :ref:`sink buffer
<sink_types:filesink>` does not enable retries.

Use ``StreamSink::safe_fwrite_unbuffered()`` directly only after the same checked ``_IONBF`` setup.
``StreamSink::safe_fwrite()`` reports short writes normally. A C library can discard buffered data on
error, so retrying ``fwrite()`` cannot recover those bytes reliably.

AndroidSink
~~~~~~~~~~~

:cpp:class:`AndroidSink` writes to the Android log through the NDK logging API.

NullSink
~~~~~~~~

:cpp:class:`NullSink` discards messages. Use it for performance tests or to silence a logger while keeping
its log calls in place.

SystemdSink
~~~~~~~~~~~

:cpp:class:`SystemdSink` writes to the systemd journal. Install ``systemd-devel`` and link with ``libsystemd``.

.. code:: cmake

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SYSTEMD REQUIRED libsystemd)
    target_link_libraries(${TARGET} ${SYSTEMD_LIBRARIES})

The journal headers share the :ref:`syslog macro names <sink_types:syslogsink>`. This example uses prefixed macros.

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
