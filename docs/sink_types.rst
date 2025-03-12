.. title:: Sink Types

Sink Types
==========

ConsoleSink
~~~~~~~~~~~

The `ConsoleSink` class sends logging output to streams `stdout` or `stderr`. Printing color codes to terminal or Windows console is also supported.

FileSink
~~~~~~~~

The :cpp:class:`FileSink` is a straightforward sink that outputs to a file. The filepath of the `FileSink` serves as a unique identifier, allowing you to retrieve the same sink later using :cpp:func:`FrontendImpl::get_sink`.

Each file can only have a single instance of `FileSink`.

.. literalinclude:: examples/quill_docs_example_file.cpp
   :language: cpp
   :linenos:

RotatingFileSink
~~~~~~~~~~~~~~~~

The :cpp:class:`RotatingFileSink` is built on top of the `FileSink` and provides log file rotation based on specified time intervals, file sizes, or daily schedules.

.. literalinclude:: ../examples/rotating_file_logging.cpp
   :language: cpp
   :linenos:

JsonFileSink/JsonConsoleSink
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. literalinclude:: examples/quill_docs_example_json_logging.cpp
   :language: cpp
   :linenos:

RotatingJsonFileSink
~~~~~~~~~~~~~~~~~~~~

The :cpp:class:`RotatingJsonFileSink` is built on top of the `JsonFileSink` and provides log file rotation based on specified time intervals, file sizes, or daily schedules.

.. literalinclude:: ../examples/rotating_json_file_logging.cpp
   :language: cpp
   :linenos:

SyslogSink
~~~~~~~~~~

The :cpp:class:`SyslogSink` leverages the syslog API to send messages.

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
        "id_1", []()
        {
          quill::SyslogSinkConfig config;
          config.set_identifier("quill");
          return config;
        }());

      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(sink));

      QUILL_LOG_INFO(logger, "A {} message with number {}", "log", 1);
      QUILL_LOG_WARNING(logger, "test message {}", 123);
    }
