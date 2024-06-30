.. title:: Basic Example

Basic Example
=============

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/ConsoleSink.h"

    int main()
    {
      // Start the backend thread
      quill::Backend::start();

      // Frontend
      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

      LOG_INFO(logger, "This is a log info example {}", 123);
    }

In the example above, a console `Sink` is created and passed to a `Logger` with its name set to 'root'.

Each `Sink` and `Logger` must be assigned a unique name during creation to facilitate retrieval later.

Each :cpp:class:`quill::Logger` contains a :cpp:class:`quill::PatternFormatter` object responsible for formatting the message.

Moreover, each :cpp:class:`quill::Logger` contains single or multiple :cpp:class:`quill::Sink` objects that deliver the log message to their output source.

A single backend thread checks for new log messages periodically.

Starting the backend thread is the user's responsibility. The backend thread will automatically stop at the end of `main`, printing every message, as long as the application terminates gracefully.

The use of macros is unavoidable to achieve better runtime performance. The static information of a log (such as format string, log level, location) is created at compile time and passed along with the type of each argument to a decoding function. A template instantiation per log statement is created.