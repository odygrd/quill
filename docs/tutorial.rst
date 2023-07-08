.. _tutorial:

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

The default logger can be accessed easily by calling :cpp:func:`Logger* quill::get_logger()`.
Any newly created logger inherits the properties of the default root logger.
Log level is always set to :cpp:enumerator:`quill::LogLeveL::Info` by default.

Each :cpp:class:`quill::Logger` contains single or multiple :cpp:class:`quill::Handler` objects. The handler
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
     std::shared_ptr<quill::Handler> file_handler = quill::file_handler(filename, "w");

     // Create a logger using this handler
     quill::Logger* logger_foo = quill::create_logger("logger_foo", file_handler);

     // Because a handler already created for this filename a pointer to the existing handler is returned
     std::shared_ptr<quill::Handler> file_handler_2 = quill::file_handler(filename, "w");

     // Create a new logger using this handler
     quill::Logger* logger_bar = quill::create_logger("logger_bar", file_handler_2);

Handler Types
==================================

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
     std::shared_ptr<quill::Handler> stdout_handler_1 = quill::stdout_handler("stdout_1");

     stdout_handler_1->set_pattern(
       "%(ascii_time) [%(process)] [%(thread)] LOG_%(level_name) %(logger_name) - %(message)", // message format
       "%D %H:%M:%S.%Qms %z",     // timestamp format
       quill::Timezone::GmtTime); // timestamp's timezone

     quill::Logger* logger_foo = quill::create_logger("logger_foo", stdout_handler_1);

     // Get the stdout file handler, with another unique name
     std::shared_ptr<quill::Handler> stdout_handler_2 = quill::stdout_handler("stdout_2");

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

      quill::std::shared_ptr<Handler> file_handler = quill::file_handler(filename, "w");
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
     quill::std::shared_ptr<Handler> file_handler = quill::rotating_file_handler(base_filename, "w", 1024, 5);

     // Create a logger using this handler
     quill::Logger* logger_bar = quill::create_logger("rotating", file_handler);

     for (uint32_t i = 0; i < 15; ++i)
     {
       LOG_INFO(logger_bar, "Hello from {} {}", "rotating logger", i);
     }

     // Get an instance to the existing rotating file handler
     quill::std::shared_ptr<Handler> file_handler = quill::rotating_file_handler(base_filename);

TimeRotatingFileHandler
-----------------------

.. doxygenfunction:: quill::time_rotating_file_handler

Daily log
~~~~~~~~~~~~~~~~~~~~~

.. code:: cpp

     // Start the backend logging thread
     quill::start();

     // Create a rotating file handler which rotates daily at 02:00
     quill::std::shared_ptr<Handler> file_handler =
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
     quill::std::shared_ptr<Handler> file_handler =
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
     quill::std::shared_ptr<Handler> json_handler =
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

Filters
==================================

A Filter class that can be used for filtering log records in the backend working thread.

This is a simple way to ensure that a logger or handler will only output desired log messages.

One or several :cpp:class:`quill::FilterBase` can be added to a :cpp:class:`quill::Handler` instance using the :cpp:func:`void add_filter(std::unique_ptr<FilterBase> filter)`
The handler stores all added filters in a vector. The final log message is logged if all filters of the handler return `true`.

Filtering per handler
-----------------------

The below example logs all WARNING and higher log level messages to console and all INFO and lower level messages to a file.

.. code:: cpp

    // Filter class for our file handler
    class FileFilter : public quill::FilterBase
    {
    public:
      FileFilter() : quill::FilterBase("FileFilter"){};

      QUILL_NODISCARD bool filter(char const* thread_id, std::chrono::nanoseconds log_record_timestamp,
                                  quill::detail::LogRecordMetadata const& metadata,
                                  fmt::memory_buffer const& formatted_record) noexcept override
      {
        if (metadata.level() < quill::LogLevel::Warning)
        {
          return true;
        }
        return false;
      }
    };

    // Filter for the stdout handler
    class StdoutFilter : public quill::FilterBase
    {
    public:
      StdoutFilter() : quill::FilterBase("StdoutFilter"){};

      QUILL_NODISCARD bool filter(char const* thread_id, std::chrono::nanoseconds log_record_timestamp,
                                  quill::detail::LogRecordMetadata const& metadata,
                                  fmt::memory_buffer const& formatted_record) noexcept override
      {
        if (metadata.level() >= quill::LogLevel::Warning)
        {
          return true;
        }
        return false;
      }
    };

    int main()
    {
      // Start the logging backend thread
      quill::start();

      // Get a handler to the file
      // The first time this function is called a file handler is created for this filename.
      // Calling the function with the same filename will return the existing handler
      quill::std::shared_ptr<Handler> file_handler = quill::file_handler("example_filters.log", "w");

      // Create and add the filter to our handler
      file_handler->add_filter(std::make_unique<FileFilter>());

      // Also create an stdout handler
      quill::std::shared_ptr<Handler> stdout_handler = quill::stdout_handler("stdout_1");

      // Create and add the filter to our handler
      stdout_handler->add_filter(std::make_unique<StdoutFilter>());

      // Create a logger using this handler
      quill::Logger* logger = quill::create_logger("logger", {file_handler, stdout_handler});

      // Change the LogLevel to print everything
      logger->set_log_level(quill::LogLevel::TraceL3);

      // Log any message ..
    }

Formatters
==================================
The :cpp:class:`quill::PatternFormatter` specifies the layout of log records in the final output.

Each :cpp:class:`quill::Handler` object owns a PatternFormatter object.
This means that each Handler can be customised to output in a different format.

Customising the format output only be done prior to the creation of the
logger by calling :cpp:func:`inline void Handler::set_pattern(std::string const &format_pattern, std::string const &timestamp_format = std::string{"%H:%M:%S.%Qns"}, Timezone timezone = Timezone::LocalTime)`.

The following format is used by default :

``ascii_time [thread_id] filename:line level_name logger_name - message``

If no custom format is set each newly created Handler uses the same formatting as the default logger.

The format output can be customised by providing a string of certain
attributes.

+-------------------+----------------+---------------------------------+
| Name              | Format         | Description                     |
+===================+================+=================================+
| ascii_time        | %(ascii_time)  | Human-readable time when the    |
|                   |                | LogRecord was created. By       |
|                   |                | default this is of the form     |
|                   |                | ‘2003-07-08 16:49:45.896’ (the  |
|                   |                | numbers after the period are    |
|                   |                | millisecond portion of the      |
|                   |                | time).                          |
+-------------------+----------------+---------------------------------+
| filename          | %(filename)    | Filename portion of pathname.   |
+-------------------+----------------+---------------------------------+
| function_name     | %(             | Name of function containing the |
|                   | function_name) | logging call.                   |
+-------------------+----------------+---------------------------------+
| level_name        | %(level_name)  | Text logging level for the      |
|                   |                | message (‘TRACEL3’, ‘TRACEL2’,  |
|                   |                | ‘TRACEL1’, ‘DEBUG’, ‘INFO’,     |
|                   |                | ‘WARNING’, ‘ERROR’, ‘CRITICAL’, |
|                   |                | ‘BACKTRACE’).                   |
+-------------------+----------------+---------------------------------+
| level_id          | %(level_id)    | Abbreviated level name (‘T3’,   |
|                   |                | ‘T2’, ‘T1’, ‘D’, ‘I’, ‘W’, ‘E’, |
|                   |                | ‘C’, ‘BT’).                     |
+-------------------+----------------+---------------------------------+
| lineno            | %(lineno)      | Source line number where the    |
|                   |                | logging call was issued (if     |
|                   |                | available).                     |
+-------------------+----------------+---------------------------------+
| message           | %(message)     | The logged message, computed as |
|                   |                | msg % args. This is set when    |
|                   |                | Formatter.format() is invoked.  |
+-------------------+----------------+---------------------------------+
| logger_name       | %(logger_name) | Name of the logger used to log  |
|                   |                | the call.                       |
+-------------------+----------------+---------------------------------+
| pathname          | %(pathname)    | Full pathname of the source     |
|                   |                | file where the logging call was |
|                   |                | issued (if available).          |
+-------------------+----------------+---------------------------------+
| thread            | %(thread)      | Thread ID (if available).       |
+-------------------+----------------+---------------------------------+
| thread name       | %(thread_name) | Thread name if set. The name of |
|                   |                | the thread must be set prior to |
|                   |                | issuing any log statement on    |
|                   |                | that thread.                    |
+-------------------+----------------+---------------------------------+
| process           | %(process)     | Process ID                      |
+-------------------+----------------+---------------------------------+

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

Setting a default formatter for logging to stdout
----------------------------------------------------------

.. code:: cpp

     // Get the stdout file handler
     quill::std::shared_ptr<Handler> console_handler = quill::stdout_handler();

     // Set a custom formatter for this handler
     console_handler->set_pattern("%(ascii_time) [%(process)] [%(thread)] %(logger_name) - %(message)", // format
                               "%D %H:%M:%S.%Qms %z",     // timestamp format
                               quill::Timezone::GmtTime); // timestamp's timezone

     // Config using the custom ts class and the stdout handler
     quill::Config cfg;
     cfg.default_handlers.emplace_back(console_handler);
     quill::configure(cfg);

     // Start the backend logging thread
     quill::start();

     // Log using the default logger
     LOG_INFO(quill::get_logger(), "The default logger is using a custom format");

     // Obtain a new logger. Since no handlers were specified during the creation of the new logger. The new logger will use the default logger's handlers. In that case it will use the stdout_handler with the modified format.
     quill::Logger* logger_foo = quill::create_logger("logger_foo");

     LOG_INFO(logger_foo, "The new logger is using the custom format");

Setting a default formatter on a FileHandler
----------------------------------------------------------

.. code:: cpp

     // Start the logging backend thread
     quill::start();

     // Calling the function with the same filename will return the existing handler
     quill::std::shared_ptr<Handler> file_handler = quill::file_handler(filename, "w");

     // Set a custom pattern to this file handler
     file_handler->set_pattern("%(ascii_time) [%(process)] [%(thread)] %(logger_name) - %(message)", // format
                               "%D %H:%M:%S.%Qms %z",     // timestamp format
                               quill::Timezone::GmtTime); // timestamp's timezone

     // Create a logger using this handler
     quill::Logger* logger_foo = quill::create_logger("logger_foo", file_handler);

     // Log using the logger
     LOG_INFO(logger_foo, "Hello from {}", "library foo");

Logger
==============

Quill creates a default :cpp:class:`quill::Logger` with a ``stdout`` handler with name as ``root``.
The default logger can be accessed easily by calling :cpp:func:`Logger* quill::get_logger()`

The default logger can be easily customised by replacing its instance
with another logger. It is possible to change the handler of the default
logger and the formatter of the default logger This however has to be
done in the beginning before the logger is used.

Logger creation
-----------------------------

New logger instances can be created by the user with the desired name, handlers and formatter.
The logger object are never instantiated directly. Instead they first have to get created by calling

Based on the create function that was used the new logger might inherit all properties of the default logger or get created with it’s own custom properties.

.. doxygenfunction:: quill::create_logger(std::string const &logger_name, std::optional<TimestampClockType> timestamp_clock_type = std::nullopt, std::optional<TimestampClock*> timestamp_clock = std::nullopt)
.. doxygenfunction:: quill::create_logger(std::string const &logger_name, Handler *handler, std::optional<TimestampClockType> timestamp_clock_type = std::nullopt, std::optional<TimestampClock*> timestamp_clock = std::nullopt)
.. doxygenfunction:: quill::create_logger(std::string const &logger_name, std::initializer_list<std::shared_ptr<Handler>> handlers, std::optional<TimestampClockType> timestamp_clock_type = std::nullopt, std::optional<TimestampClock*> timestamp_clock = std::nullopt)
.. doxygenfunction:: quill::create_logger(std::string const &logger_name, std::vector<std::shared_ptr<Handler>> const &handlers, std::optional<TimestampClockType> timestamp_clock_type = std::nullopt, std::optional<TimestampClock*> timestamp_clock = std::nullopt)

Logger access
-----------------------------

.. doxygenfunction:: quill::get_logger
.. doxygenfunction:: quill::get_root_logger

Create single handler logger
-----------------------------

.. code:: cpp

     // Get a handler to a file
     quill::std::shared_ptr<Handler> file_handler = quill::file_handler("example.log", "w");

     // Create a logger using this handler
     quill::Logger* logger_foo = quill::create_logger("logger_foo", file_handler);

     LOG_INFO(logger_foo, "Hello from {}", "library foo");

Create multi handler logger
-----------------------------

.. code:: cpp

     // Get a handler to a file
     quill::std::shared_ptr<Handler> file_handler = quill::file_handler(filename, "w");

     // Get a handler to stdout
     quill::std::shared_ptr<Handler> stdout_handler = quill::stdout_handler();

     // Create a logger using both handlers
     quill::Logger* logger_foo = quill::create_logger("logger_foo", {file_handler, quill::stdout_handler()});

     LOG_INFO(logger_foo, "Hello from {}", "library foo");

Avoiding the use of Logger objects
---------------------------------------
For some applications the use of the single root logger might be enough. In that case passing the logger everytime
to the macro becomes inconvenient. The solution is to overwrite the quill macros with your own macros.

.. code:: cpp

    #define MY_LOG_INFO(fmt, ...) QUILL_LOG_INFO(quill::get_root_logger(), fmt, ##__VA_ARGS__)

Or you can simply define

.. c:macro:: QUILL_ROOT_LOGGER_ONLY

.. code:: cpp

    #define QUILL_ROOT_LOGGER_ONLY
    #include "quill/Quill.h"

    int main()
    {
      quill::start();

      // because we defined QUILL_ROOT_LOGGER_ONLY we do not have to pass a logger* anymore, the root logger is always used
      LOG_INFO("Hello {}", "world");
      LOG_ERROR("This is a log error example {}", 7);
  }

Backtrace Logging
====================

Backtrace logging enables log messages to be stored in a ring buffer and either

- displayed later on demand or
- when a high severity log message is logged

Backtrace logging needs to be enabled first on the instance of :cpp:class:`quill::Logger`

.. doxygenfunction:: init_backtrace
.. doxygenfunction:: flush_backtrace

.. note:: Backtrace log messages store the original timestamp of the message. Since they are kept and flushed later the timestamp in the log file will be out of order

Store messages in the ring buffer and display them when ``LOG_ERROR`` is logged
--------------------------------------------------------------------------------------------------------------------

.. code:: cpp

       // Loggers can store in a ring buffer messages with LOG_BACKTRACE and display later when e.g.
       // a LOG_ERROR message was logged from this logger

       quill::Logger* logger = quill::create_logger("example_1");

       // Enable the backtrace with a max ring buffer size of 2 messages which will get flushed when
       // a LOG_ERROR(...) or higher severity log message occurs via this logger.
       // Backtrace has to be enabled only once in the beginning before calling LOG_BACKTRACE(...) for the first time.
       logger->init_backtrace(2, quill::LogLevel::Error);

       LOG_INFO(logger, "BEFORE backtrace Example {}", 1);
       LOG_BACKTRACE(logger, "Backtrace log {}", 1);
       LOG_BACKTRACE(logger, "Backtrace log {}", 2);
       LOG_BACKTRACE(logger, "Backtrace log {}", 3);
       LOG_BACKTRACE(logger, "Backtrace log {}", 4);

       // Backtrace is not flushed yet as we requested to flush on errors
       LOG_INFO(logger, "AFTER backtrace Example {}", 1);

       // log message with severity error - This will also flush the backtrace which has 2 messages
       LOG_ERROR(logger, "An error has happened, Backtrace is also flushed.");

       // The backtrace is flushed again after LOG_ERROR but in this case it is empty
       LOG_ERROR(logger, "An second error has happened, but backtrace is now empty.");

       // Log more backtrace messages
       LOG_BACKTRACE(logger, "Another Backtrace log {}", 1);
       LOG_BACKTRACE(logger, "Another Backtrace log {}", 2);

       // Nothing is logged at the moment
       LOG_INFO(logger, "Another log info");

       // Still nothing logged - the error message is on a different logger object
       quill::Logger* logger_2 = quill::create_logger("example_1_1");
       LOG_CRITICAL(logger_2, "A critical error from different logger.");

       // The new backtrace is flushed again due to LOG_CRITICAL
       LOG_CRITICAL(logger, "A critical error from the logger we had a backtrace.");

::

   13:02:03.405589220 [196635] example_backtrace.cpp:18 LOG_INFO      example_1 - BEFORE backtrace Example 1
   13:02:03.405617051 [196635] example_backtrace.cpp:30 LOG_INFO      example_1 - AFTER backtrace Example 1
   13:02:03.405628045 [196635] example_backtrace.cpp:33 LOG_ERROR     example_1 - An error has happened, Backtrace is also flushed.
   13:02:03.405608746 [196635] example_backtrace.cpp:26 LOG_BACKTRACE example_1 - Backtrace log 3
   13:02:03.405612082 [196635] example_backtrace.cpp:27 LOG_BACKTRACE example_1 - Backtrace log 4
   13:02:03.405648711 [196635] example_backtrace.cpp:36 LOG_ERROR     example_1 - An second error has happened, but backtrace is now empty.
   13:02:03.405662233 [196635] example_backtrace.cpp:43 LOG_INFO      example_1 - No errors so far
   13:02:03.405694451 [196635] example_backtrace.cpp:47 LOG_CRITICAL  example_1_1 - A critical error from different logger.
   13:02:03.405698838 [196635] example_backtrace.cpp:50 LOG_CRITICAL  example_1 - A critical error from the logger we had a backtrace.

Store messages in the ring buffer and display them on demand
--------------------------------------------------------------------------------------------------------------------

.. code:: cpp

       quill::Logger* logger = quill::create_logger("example_2");

       // Store maximum of two log messages. By default they will never be flushed since no LogLevel severity is specified
       logger->init_backtrace(2);

       LOG_INFO(logger, "BEFORE backtrace Example {}", 2);

       LOG_BACKTRACE(logger, "Backtrace log {}", 100);
       LOG_BACKTRACE(logger, "Backtrace log {}", 200);
       LOG_BACKTRACE(logger, "Backtrace log {}", 300);

       LOG_INFO(logger, "AFTER backtrace Example {}", 2);

       // an error has happened - flush the backtrace manually
       logger->flush_backtrace();

User Defined Types
========================

Quill does asynchronous logging. When a user defined type has to be logged, the copy constructor is called and the
formatting is performed on a backend logging thread via a call to :cpp:func:`ostream& operator<<(ostream& os, T const& t)`

This creates issues with user defined types that contain mutable references, raw pointers that can be mutated or for
example a :cpp:func:`std::shared_ptr` that can be modified.

By default a compile time check is performed that checks for unsafe to copy types.

Many user defined types including STL containers, tuples and pairs of build in types are automatically detected in
compile type as safe to copy and will pass the check.

The following code gives a good idea of the types that by default are safe to get copied

.. code:: cpp

    struct filter_copyable : std::disjunction<std::is_arithmetic<T>,
                                         is_string<T>,
                                         std::is_trivial<T>,
                                         is_user_defined_copyable<T>,
                                         is_user_registered_copyable<T>,
                                         is_copyable_pair<T>,
                                         is_copyable_tuple<T>,
                                         is_copyable_container<T>
                                         >

The following types is just a small example of detectable safe-to-copy types

.. code:: cpp

       std::vector<std::vector<std::vector<int>>>;
       std::tuple<int,bool,double,float>>;
       std::pair<char, double>;
       std::tuple<std::vector<std::string>, std::map<int, std::sting>>;

.. note:: Passing pointers for logging is not permitted by libfmt in compile time, with the only exception being ``void*``. Therefore they are excluded from the above check.

Requirements
-------------------

To log a user defined type the following requirements must met:

- The type has to be copy constructible
- Specialize `fmt::formatter<T>` and implement parse and format methods (`see here <http://fmt.dev/latest/api.html#formatting-user-defined-types>`_) or provide an overloaded insertion operator (`see here <https://fmt.dev/latest/api.html#std-ostream-support>`_)

Logging user defined types in default mode
---------------------------------------------------------

In default mode copying non-trivial user defined types is *not*
permitted unless they are tagged as safe to copy

Consider the following example :

.. code:: cpp

       class User
       {
       public:
         User(std::string name) : name(std::move(name)){};

         friend std::ostream& operator<<(std::ostream& os, User2 const& obj)
         {
           os << "name : " << obj.name;
           return os;
         }
       private:
         std::string name;
       };

      int main()
      {
        User user{"Hello"};
        LOG_INFO(quill::get_logger(), "The user is {}", usr);
      }

The above log statement would fail with a compiler error. The type is
non-trivial, there is no way to automatically detect the type is safe to
copy.

To log this user defined type we have two options:
 - call :cpp:func:`operator<<` on the caller hot path and pass a :cpp:func:`std::string` to the logger if the type contains mutable references and is not safe to copy
 - mark the type as ``safe to copy`` and let the backend logger thread do the formatting if the type is safe to copy

Registering or tagging user defined types as ``safe to copy``
-------------------------------------------------------------

It is possible to mark the class as safe to copy and the logger will attempt to copy it.
In this case the user defined type will get copied.

.. note:: It is the responsibility of the user to ensure that the class does not contain mutable references or pointers before tagging it as safe

There are 2 different ways to do that :

1) Specialize :cpp:func:`copy_loggable<T>`

.. code:: cpp

       class User
       {
       public:
         User(std::string name) : name(std::move(name)){};

         friend std::ostream& operator<<(std::ostream& os, User2 const& obj)
         {
           os << "name : " << obj.name;
           return os;
         }

       private:
         std::string name;
       };

       /** Registered as safe to copy **/
       namespace quill {
         template <>
         struct copy_loggable<User> : std::true_type { };
       }

       int main()
       {
         User user{"Hello"};
         LOG_INFO(quill::get_logger(), "The user is {}", usr);
       }

2) Use .. c:macro:: `QUILL_COPY_LOGGABLE` macro inside your class definition. This is not preferable as you need to edit the class to provide that

.. code:: cpp

       class User
       {
       public:
         User(std::string name) : name(std::move(name)){};

         friend std::ostream& operator<<(std::ostream& os, User2 const& obj)
         {
           os << "name : " << obj.name;
           return os;
         }

         QUILL_COPY_LOGGABLE; /** Tagged as safe to copy **/

       private:
         std::string name;
       };

       int main()
       {
         User user{"Hello"};
         LOG_INFO(quill::get_logger(), "The user is {}", usr);
       }

Then the following will compile, the user defined type will get copied, and :cpp:func:`ostream& operator<<(ostream& os, T const& t)` will be called in the background thread.

Generally speaking, tagging functionality in this mode exists to also
make the user thinks about the user defined type they are logging.
It has to be maintained when a new class member is added. If the log level
severity of the log statement is below ``INFO`` you might as well
consider formatting the type to a string in the caller path instead of
maintaining a safe-to-copy tag.

Logging non-copy constructible or unsafe to copy user defined types
-------------------------------------------------------------------

Consider the following unsafe to copy user defined type. In this case we want to format on the caller thread.

This has to be explicitly done by the user as it might be expensive.

There is a utility function offered or users can write their own routine.

.. doxygenfunction:: quill::utility::to_string

.. code:: cpp

   #include "quill/Quill.h"
   #include "quill/Utility.h"

   class User
   {
   public:
     User(std::string* name) : name(name){};

     friend std::ostream& operator<<(std::ostream& os, User const& obj)
     {
       os << "name : " << obj.name;
       return os;
     }

   private:
     std::string* name;
   };

   int main()
   {
     auto str = std::make_unique<std::string>("User A");
     User usr{str.get()};

     // We format the object in the hot path because it is not safe to copy this kind of object
     LOG_INFO(quill::get_logger(), "The user is {}", quill::utility::to_string(usr));

     // std::string* is modified - Here the backend worker receives a copy of User but the pointer to
     // std::string* is still shared and mutated in the below line
     str->replace(0, 1, "T");
   }

Logging in ``QUIL_MODE_UNSAFE``
--------------------------------------------------------

When QUIL_MODE_UNSAFE is enabled, Quill will *not check* in compile time for safe to copy user defined types.

All types will are copied unconditionally in this mode as long as they are copy constructible. This mode is not
recommended as the user has to be extremely careful about any user define type they are logging.

However, it is there for users who don’t want to tag their types.

The following example compiles and copies the user defined type even tho it is a non-trivial type.

.. code:: cpp

       #define QUIL_MODE_UNSAFE
       #include "quill/Quill.h"

       class User
       {
       public:
         User(std::string name) : name(std::move(name)){};

         friend std::ostream& operator<<(std::ostream& os, User2 const& obj)
         {
           os << "name : " << obj.name;
           return os;
         }
       private:
         std::string name;
       };

       int main()
       {
         User user{"Hello"};
         LOG_INFO(quill::get_logger(), "The user is {}", usr);
       }
