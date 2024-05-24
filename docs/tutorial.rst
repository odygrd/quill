.. _tutorial:

##############################################################################
Tutorial
##############################################################################

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

In the above example a logger to ``stdout`` is created with it's name set to “root”.

Each :cpp:class:`quill::LoggerImpl` contains a :cpp:class:`quill::PatternFormatter` object which is responsible for the
formatting of the message.

Moreover, each :cpp:class:`quill::LoggerImpl` contains single or multiple :cpp:class:`quill::Sink` objects. The sink
objects actually deliver the log message to their output source.

A single backend thread is checking for new log messages periodically.

Starting the backend thread is the responsibility of the user. The backend thread will automatically stop at the end
of `main` printing every message, as long as the application is terminated gracefully.

Use of macros is unavoidable in order to achieve better runtime performance. The static information of a log
(such as format string, log level, location) is created in compile time. It is passed along with the type of each
argument to a decoding function. A template instantiation per log statement is created.

Logging Macros
================

The following macros are provided for logging:

.. c:macro:: LOG_TRACE_L3(logger, log_message_format, args)
.. c:macro:: LOG_TRACE_L2(logger, log_message_format, args)
.. c:macro:: LOG_TRACE_L1(logger, log_message_format, args)
.. c:macro:: LOG_DEBUG(logger, log_message_format, args)
.. c:macro:: LOG_INFO(logger, log_message_format, args)
.. c:macro:: LOG_WARNING(logger, log_message_format, args)
.. c:macro:: LOG_ERROR(logger, log_message_format, args)
.. c:macro:: LOG_CRITICAL(logger, log_message_format, args)
.. c:macro:: LOG_BACKTRACE(logger, log_message_format, args)

Sinks
========

Sinks are the objects responsible for writing logs to their respective targets.

A :cpp:class:`quill::Sink` object serves as the base class for various sink-derived classes.

Each sink handles outputting logs to a single target, such as a file, console, or database.

Upon creation, a sink object is registered and owned by a central manager object, the quill::detail::SinkManager.

For files, one sink is created per filename, and the file is opened once. If a sink is requested that refers to an already opened file, the existing Sink object is returned. Users can create multiple stdout or stderr handles by providing a unique ID per handle.

When creating a logger, one or more sinks for that logger can be specified. Sinks can only be registered during the logger creation.

Sharing sinks between loggers
==================================

It is possible to share the same sink object between multiple logger objects.
For example when all logger objects are writing to the same file. The following code is also thread-safe.

.. code:: cpp

     auto file_sink = Frontend::create_or_get_sink<FileSink>(
       filename,
       []()
       {
         FileSinkConfig cfg;
         cfg.set_open_mode('w');
         return cfg;
       }(),
       FileEventNotifier{});

     quill::Logger* logger_a = Frontend::create_or_get_logger("logger_a", file_sink);
     quill::Logger* logger_b = Frontend::create_or_get_logger("logger_b", file_sink);

Sink Types
==================================

ConsoleSink
--------------

The ``ConsoleSink`` class sends logging output to streams ``stdout`` or ``stderr``.
Printing colour codes to terminal or windows console is also supported.

FileHandler
-----------

Logging to file
~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

    int main()
    {
      // Start the backend thread
      quill::Backend::start();

      // Frontend
      auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "trivial_logging.log",
        []()
        {
          quill::FileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
          return cfg;
        }(),
        quill::FileEventNotifier{});

      quill::Logger* logger =
        quill::Frontend::create_or_get_logger("root", std::move(file_sink));

      LOG_INFO(logger, "log something {}", 123);
    }

RotatingFileSink
-------------------

Rotating log by size or time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

      // Start the backend thread
      quill::Backend::start();

      // Frontend
      auto rotating_file_sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
        "rotating_file.log",
        []()
        {
          // See RotatingFileSinkConfig for more options
          quill::RotatingFileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
          cfg.set_rotation_time_daily("18:30");
          cfg.set_rotation_max_file_size(1024);
          return cfg;
        }());

      quill::Logger* logger =
        quill::Frontend::create_or_get_logger("root", std::move(rotating_file_sink));

      for (int i = 0; i < 20; ++i)
      {
        LOG_INFO(logger, "Hello from rotating logger, index is {}", i);
      }

JsonFileSink
-----------------------

Json log
~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

      // Start the backend thread
      quill::Backend::start();

      // Frontend

      // Create a json file for output
      auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonFileSink>(
        "json_sink_logging.log",
        []()
        {
          quill::JsonFileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
          return cfg;
        }(),
        quill::FileEventNotifier{});

      // When using the JsonFileSink, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
      quill::Logger* json_logger = quill::Frontend::create_or_get_logger(
        "json_logger", std::move(json_sink), "", "%H:%M:%S.%Qns", quill::Timezone::GmtTime);

      for (int i = 0; i < 2; ++i)
      {
        LOG_INFO(json_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
      }

      // It is also possible to create a logger than logs to both the json file and stdout
      // with the appropriate format
      auto json_sink_2 = quill::Frontend::get_sink("json_sink_logging.log");
      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console_sink_id_1");

      // We set a custom format pattern here to also include the named_args
      quill::Logger* hybrid_logger = quill::Frontend::create_or_get_logger(
        "hybrid_logger", {std::move(json_sink_2), std::move(console_sink)},
        "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<20) "
        "%(message) [%(named_args)]");

      for (int i = 2; i < 4; ++i)
      {
        LOG_INFO(hybrid_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
      }

Filters
==================================

A Filter class that can be used for filtering log records in the backend working thread.

This is a simple way to ensure that a logger or sink will only output desired log messages.

One or several :cpp:class:`quill::Filter` can be added to a :cpp:class:`quill::Sink` instance using the
:cpp:func:`void add_filter(std::unique_ptr<Filter> filter)`

The sink stores all added filters in a vector. The final log message is logged if all filters of the sink return `true`.

Filtering per sink
-----------------------

The below example logs all WARNING and higher log level messages to console and all INFO and lower level messages to a file.

.. code:: cpp

        // Filter class for our file sink
        class FileFilter : public quill::Filter
        {
        public:
          FileFilter() : quill::Filter("FileFilter"){};

          QUILL_NODISCARD bool filter(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp, std::string_view thread_id,
                                      std::string_view thread_name, std::string_view logger_name,
                                      quill::LogLevel log_level, std::string_view log_message) noexcept override
          {
            if (log_metadata->log_level() < quill::LogLevel::Warning)
            {
              return true;
            }
            return false;
          }
        };

        // Filter for the stdout sink
        class StdoutFilter : public quill::Filter
        {
        public:
          StdoutFilter() : quill::Filter("StdoutFilter"){};

          QUILL_NODISCARD bool filter(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp, std::string_view thread_id,
                                      std::string_view thread_name, std::string_view logger_name,
                                      quill::LogLevel log_level, std::string_view log_message) noexcept override
          {
            if (log_metadata->log_level() >= quill::LogLevel::Warning)
            {
              return true;
            }
            return false;
          }
        };

        int main()
        {
          // Start the logging backend thread
          quill::Backend::start();

          // Get a sink to the file
          // The first time this function is called a file sink is created for this filename.
          // Calling the function with the same filename will return the existing sink
          auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
            "example_filters.log",
            []()
            {
              quill::FileSinkConfig cfg;
              cfg.set_open_mode('w');
              cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
              return cfg;
            }(),
            quill::FileEventNotifier{});

          // Create and add the filter to our sink
          file_sink->add_filter(std::make_unique<FileFilter>());

          // Also create an stdout sink
          auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

          // Create and add the filter to our sink
          console_sink->add_filter(std::make_unique<StdoutFilter>());

          // Create a logger using this sink
          quill::Logger* logger = quill::Frontend::create_or_get_logger("logger", {std::move(file_sink), std::move(console_sink)});

          LOG_INFO(logger, "test");
          LOG_ERROR(logger, "test");
        }

Formatters
==================================
The :cpp:class:`quill::PatternFormatter` specifies the layout of log records in the final output.

Each :cpp:class:`quill::LoggerImpl` object owns a PatternFormatter object.
This means that each Logger can be customised to output in a different format.

Customising the format output only be done during the creation of the logger.

If no custom format is set each newly created Sink uses the same formatting as the default logger.

The format output can be customised by providing a string of certain
attributes.

+-------------------------+--------------------------+----------------------------------------+
| Name                    | Format                   | Description                            |
+=========================+==========================+========================================+
| time                    | %(time)                  | Human-readable time when the LogRecord |
|                         |                          | was created. By default this is of the |
|                         |                          | form '2003-07-08 16:49:45.896' (the    |
|                         |                          | numbers after the period are the       |
|                         |                          | millisecond portion of the time).      |
+-------------------------+--------------------------+----------------------------------------+
| file_name               | %(file_name)             | Filename portion of pathname.          |
+-------------------------+--------------------------+----------------------------------------+
| full_path               | %(full_path)             | Full path of the source file where the |
|                         |                          | logging call was issued.               |
+-------------------------+--------------------------+----------------------------------------+
| caller_function         | %(caller_function)       | Name of function containing the        |
|                         |                          | logging call.                          |
+-------------------------+--------------------------+----------------------------------------+
| log_level               | %(log_level)             | Text logging level for the message     |
|                         |                          | (‘TRACEL3’, ‘TRACEL2’, ‘TRACEL1’,      |
|                         |                          | ‘DEBUG’, ‘INFO’, ‘WARNING’, ‘ERROR’,   |
|                         |                          | ‘CRITICAL’, ‘BACKTRACE’).              |
+-------------------------+--------------------------+----------------------------------------+
| log_level_id            | %(log_level_id)          | Abbreviated level name (‘T3’, ‘T2’,    |
|                         |                          | ‘T1’, ‘D’, ‘I’, ‘W’, ‘E’, ‘C’, ‘BT’).  |
+-------------------------+--------------------------+----------------------------------------+
| line_number             | %(line_number)           | Source line number where the logging   |
|                         |                          | call was issued (if available).        |
+-------------------------+--------------------------+----------------------------------------+
| logger                  | %(logger)                | Name of the logger used to log the     |
|                         |                          | call.                                  |
+-------------------------+--------------------------+----------------------------------------+
| message                 | %(message)               | The logged message, computed as msg %  |
|                         |                          | args. This is set when Formatter.      |
|                         |                          | format() is invoked.                   |
+-------------------------+--------------------------+----------------------------------------+
| thread_id               | %(thread_id)             | Thread ID (if available).              |
+-------------------------+--------------------------+----------------------------------------+
| thread_name             | %(thread_name)           | Thread name if set. The name of the    |
|                         |                          | thread must be set prior to issuing    |
|                         |                          | any log statement on that thread.      |
+-------------------------+--------------------------+----------------------------------------+
| process_id              | %(process_id)            | Process ID                             |
+-------------------------+--------------------------+----------------------------------------+
| source_location         | %(source_location)       | Full source file path and line number  |
|                         |                          | as a single string                     |
+-------------------------+--------------------------+----------------------------------------+
| short_source_location   | %(short_source_location) | Full source file path and line         |
|                         |                          | number as a single string              |
+-------------------------+--------------------------+----------------------------------------+
| tags                    | %(tags)                  | Additional custom tags appended to the |
|                         |                          | message when _WITH_TAGS macros are     |
|                         |                          | used.                                  |
+-------------------------+--------------------------+----------------------------------------+
| named_args              | %(named_args)            | Key-value pairs appended to the        |
|                         |                          | message. Only applicable with          |
|                         |                          | for a named args log format;           |
|                         |                          | remains empty otherwise.               |
+-------------------------+--------------------------+----------------------------------------+


Customising the timestamp
-----------------------------

The timestamp is customisable by :

- Format. Same format specifiers as ``strftime(...)`` format without the additional ``.Qms`` ``.Qus`` ``.Qns`` arguments.
- Local timezone or GMT timezone. Local timezone is used by default.
- Fractional second precision. Using the additional fractional second specifiers in the timestamp format string.

========= ============
Specifier Description
========= ============
%Qms      Milliseconds
%Qus      Microseconds
%Qns      Nanoseconds
========= ============

By default ``"%H:%M:%S.%Qns"`` is used.

.. note:: MinGW does not support all ``strftime(...)`` format specifiers and you might get a ``bad alloc`` if the format specifier is not supported

Setting a custom format for logging to stdout
----------------------------------------------------------

.. code:: cpp

  quill::Logger* logger =
    quill::Frontend::create_or_get_logger("root", std::move(sink),
                                          "%(time) [%(thread_id)] %(short_source_location:<28) "
                                          "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                          "%H:%M:%S.%Qns", quill::Timezone::GmtTime);

Logger
-----------------------------

Logger instances can be created by the user with the desired name, sinks and formatter.
The logger object are never instantiated directly. Instead they first have to get created

:cpp:func:`Frontend::create_or_get_logger(std::string const& logger_name, std::shared_ptr<Sink> sink, std::string const& format_pattern = "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) %(message)", std::string const& time_pattern = "%H:%M:%S.%Qns", Timezone timestamp_timezone = Timezone::LocalTime, ClockSourceType clock_source = ClockSourceType::Tsc, UserClockSource* user_clock = nullptr)`

:cpp:func:`Frontend::create_or_get_logger(std::string const& logger_name, std::initializer_list<std::shared_ptr<Sink>> sinks, std::string const& format_pattern = "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) %(message)", std::string const& time_pattern = "%H:%M:%S.%Qns", Timezone timestamp_timezone = Timezone::LocalTime, ClockSourceType clock_source = ClockSourceType::Tsc, UserClockSource* user_clock = nullptr)`

Logger access
-----------------------------

:cpp:func:`Frontend::get_logger(std::string const& name)`

Logger creation
-----------------------------

.. code:: cpp

     auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

     quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

     LOG_INFO(logger, "Hello from {}", "library foo");

Avoiding the use of Logger objects
---------------------------------------
For some applications the use of the single root logger might be enough. In that case passing the logger everytime
to the macro becomes inconvenient. The solution is to store the created Logger as a static variable and create your
own macros. See `example <https://github.com/odygrd/quill/blob/master/examples/recommended_usage/quill_wrapper/include/quill_wrapper/overwrite_macros.h>`_

Backtrace Logging
====================

Backtrace logging enables log messages to be stored in a ring buffer and either

- displayed later on demand or
- when a high severity log message is logged

Backtrace logging needs to be enabled first on the instance of :cpp:class:`quill::LoggerImpl`

.. doxygenfunction:: init_backtrace
.. doxygenfunction:: flush_backtrace

.. note:: Backtrace log messages store the original timestamp of the message. Since they are kept and flushed later the
timestamp in the log file will be out of order.

.. note:: Backtrace log messages are still pushed to the SPSC queue from the frontend to the backend.

Store messages in the ring buffer and display them when ``LOG_ERROR`` is logged
--------------------------------------------------------------------------------------------------------------------

.. code:: cpp

    // a LOG_ERROR(...) or higher severity log message occurs via this logger.
    // Enable the backtrace with a max ring buffer size of 2 messages which will get flushed when
    // Backtrace has to be enabled only once in the beginning before calling LOG_BACKTRACE(...) for the first time.
    logger->init_backtrace(2, quill::LogLevel::Error);

    LOG_INFO(logger, "BEFORE backtrace Example {}", 1);

    LOG_BACKTRACE(logger, "Backtrace log {}", 1);
    LOG_BACKTRACE(logger, "Backtrace log {}", 2);
    LOG_BACKTRACE(logger, "Backtrace log {}", 3);
    LOG_BACKTRACE(logger, "Backtrace log {}", 4);

    // Backtrace is not flushed yet as we requested to flush on errors
    LOG_INFO(logger, "AFTER backtrace Example {}", 1);

    // log message with severity error - This will also flush_sink the backtrace which has 2 messages
    LOG_ERROR(logger, "An error has happened, Backtrace is also flushed.");

    // The backtrace is flushed again after LOG_ERROR but in this case it is empty
    LOG_ERROR(logger, "An second error has happened, but backtrace is now empty.");

    // Log more backtrace messages
    LOG_BACKTRACE(logger, "Another Backtrace log {}", 1);
    LOG_BACKTRACE(logger, "Another Backtrace log {}", 2);

    // Nothing is logged at the moment
    LOG_INFO(logger, "Another log info");

    // Still nothing logged - the error message is on a different logger object
    quill::LoggerImpl* logger_2 = quill::get_logger("example_1_1");

    LOG_CRITICAL(logger_2, "A critical error from different logger.");

    // The new backtrace is flushed again due to LOG_CRITICAL
    LOG_CRITICAL(logger, "A critical error from the logger we had a backtrace.");

Store messages in the ring buffer and display them on demand
--------------------------------------------------------------------------------------------------------------------

.. code:: cpp
       // Store maximum of two log messages. By default they will never be flushed since no LogLevel severity is specified
       logger->init_backtrace(2);

       LOG_INFO(logger, "BEFORE backtrace Example {}", 2);

       LOG_BACKTRACE(logger, "Backtrace log {}", 100);
       LOG_BACKTRACE(logger, "Backtrace log {}", 200);
       LOG_BACKTRACE(logger, "Backtrace log {}", 300);

       LOG_INFO(logger, "AFTER backtrace Example {}", 2);

       // an error has happened - flush_log_messages the backtrace manually
       logger->flush_backtrace();

