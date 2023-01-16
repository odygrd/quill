.. usage:

##############################################################################
Tutorial
##############################################################################

Basic Example
=============

.. code:: cpp

   #include "quill/Quill.h"

   int main()
   {
     // Start the logging backend thread
     quill::start();

     // Get a pointer to the default logger
     quill::Logger* dl = quill::get_logger();

     LOG_INFO(dl, "Welcome to Quill!");
     LOG_ERROR(dl, "An error message with error code {}, error message {}", 123, "system_error");
   }

In the above example a default logger to ``stdout`` is created with it's name set to “root”.

The default logger can be accessed easily by calling :cpp:func:`Logger* get_logger(char const* logger_name = nullptr)`
Any newly created logger inherits the properties of the default root logger.
Log level is always set to :cpp:enumerator:`quill::LogLeveL::Info` by default.

Each :cpp:class:`quill::Logger` object contains single or multiple :cpp:class:`quill::Handler` objects. The handler
objects actually deliver the log message to their. Each handler contains a :cpp:class:`quill::PatternFormatter`
object which is responsible for the formatting of the message.

A single backend thread is checking for new log messages periodically. Starting the backend thread is the responsibility
of the user. The backend thread will automatically stop at the end of `main` printing every message, as long as the
application is terminated gracefully.

Use of macros is unavoidable in order to achieve better runtime performance. The static information of a log
(such as format string, log level, location) is created in compile time. It is passed along with the type of each
argument as a template parameter to a decoding function. A template instantiation per log statement is created.

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

Handlers
========

Handlers are the objects that actually write the log to their target.

A :cpp:class:`quill::Handler` object is the base class for each different handler derived classes.

Each handler is responsible for outputting the log to a single target (e.g file, console, db), and owns a
:cpp:class:`quill::PatternFormatter` object which formats the messages to its destination.

Upon the handler creation, the handler object is registered and owned by
a central manager object the :cpp:class:`quill::detail::HandlerCollection`

For files, one handler is created per filename. For stdout and stderr a
default handler for each one is always created during initialisation. It
is possible for the user to create multiple stdout or stderr handles by
providing a unique id per handle.

When creating a custom logger one or more handlers for this logger can
be specified. This can only be done only the logger creation.

Sharing handlers between loggers
==================================

It is possible to share the same handle object between multiple logger objects.
For example when all logger objects are writing to the same file. The following code is also thread-safe.

.. code:: cpp

     // The first time this function is called a file handler is created for this filename.
     // Calling the function with the same filename will return the existing handler
     quill::Handler* file_handler = quill::file_handler(filename, "w");

     // Create a logger using this handler
     quill::Logger* logger_foo = quill::create_logger("logger_foo", file_handler);

     // Because a handler already created for this filename a pointer to the existing handler is returned
     quill::Handler* file_handler_2 = quill::file_handler(filename, "w");

     // Create a new logger using this handler
     quill::Logger* logger_bar = quill::create_logger("logger_bar", file_handler_2);

ConsoleHandler
--------------

The ``ConsoleHandler`` class sends logging output to streams ``stdout`` or ``stderr``.
Printing colour codes to terminal or windows console is also supported.

.. doxygenfunction:: quill::stdout_handler

.. doxygenfunction:: quill::stderr_handler

Creating multiple ConsoleHandler objects
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

While when operating to files, only one handle object can be created per file name, this is not the case
for ``stdout`` or ``stderr``.
It is possible to create multiple handlers to ``stdout`` or ``stderr`` by providing a unique name to each handler.

This is useful for when you want to have different loggers writing to ``stdout`` with different format.

.. code:: cpp

     // Get the stdout file handler, with a unique name
     quill::Handler* stdout_handler_1 = quill::stdout_handler("stdout_1");

     stdout_handler_1->set_pattern(
       "%(ascii_time) [%(process)] [%(thread)] LOG_%(level_name) %(logger_name) - %(message)", // message format
       "%D %H:%M:%S.%Qms %z",     // timestamp format
       quill::Timezone::GmtTime); // timestamp's timezone

     quill::Logger* logger_foo = quill::create_logger("logger_foo", stdout_handler_1);

     // Get the stdout file handler, with another unique name
     quill::Handler* stdout_handler_2 = quill::stdout_handler("stdout_2");

     stdout_handler_2->set_pattern("%(ascii_time) LOG_%(level_name) %(logger_name) - %(message)", // message format
                                   "%D %H:%M:%S.%Qms %z",     // timestamp format
                                   quill::Timezone::GmtTime); // timestamp's timezone

     quill::Logger* logger_bar = quill::create_logger("logger_bar", stdout_handler_2);

FileHandler
-----------

.. doxygenfunction:: quill::file_handler

Logging to file
~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

    int main()
    {
      quill::start();

      quill::Handler* file_handler = quill::file_handler(filename, "w");
      quill::Logger* l = quill::create_logger("logger", file_handler);

      LOG_INFO(l, "Hello World");
      LOG_INFO(quill::get_logger("logger"), "Hello World");
    }

RotatingFileHandler
-------------------

.. doxygenfunction:: quill::rotating_file_handler

Rotating log by size
~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

     // Start the backend logging thread
     quill::start();

     // Create a rotating file handler with a max file size per log file and maximum rotation up to 5 times
     quill::Handler* file_handler = quill::rotating_file_handler(base_filename, "w", 1024, 5);

     // Create a logger using this handler
     quill::Logger* logger_bar = quill::create_logger("rotating", file_handler);

     for (uint32_t i = 0; i < 15; ++i)
     {
       LOG_INFO(logger_bar, "Hello from {} {}", "rotating logger", i);
     }

     // Get an instance to the existing rotating file handler
     quill::Handler* file_handler = quill::rotating_file_handler(base_filename);

TimeRotatingFileHandler
-----------------------

.. doxygenfunction:: quill::time_rotating_file_handler

Daily log
~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

     // Start the backend logging thread
     quill::start();

     // Create a rotating file handler which rotates daily at 02:00
     quill::Handler* file_handler =
       quill::time_rotating_file_handler(filename, "w", "daily", 1, 10, Timezone::LocalTime, "02:00");

     // Create a logger using this handler
     quill::Logger* logger_bar = quill::create_logger("daily_logger", file_handler);

     LOG_INFO(logger_bar, "Hello from {}", "daily logger");

Hourly log
~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

     // Start the backend logging thread
     quill::start();

     // Create a rotating file handler which rotates every one hour and keep maximum 24 files
     quill::Handler* file_handler =
       quill::time_rotating_file_handler(filename, "w", "H", 24, 10);

     // Create a logger using this handler
     quill::Logger* logger_bar = quill::create_logger("daily_logger", file_handler);

     LOG_INFO(logger_bar, "Hello from {}", "daily logger");

JsonFileHandler
-----------------------

.. doxygenfunction:: quill::json_file_handler

Json log
~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

     quill::Config cfg;

     // use the json handler
     quill::Handler* json_handler =
       quill::json_file_handler("json_output.log", "w", quill::FilenameAppend::DateTime);

     // Change how the date is formatted in the structured log.
     // JsonFileHandler must always have an empty pattern "" as the first argument.
     json_handler->set_pattern("", std::string{"%Y-%m-%d %H:%M:%S.%Qus"});

     // set this handler as the default for any new logger we are creating
     cfg.default_handlers.emplace_back(json_handler);

     quill::configure(cfg);

     // Start the logging backend thread
     quill::start();

     // log to the json file ONLY by using the default logger
     quill::Logger* logger = quill::get_logger();
     for (int i = 0; i < 2; ++i)
     {
       LOG_INFO(logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
     }
