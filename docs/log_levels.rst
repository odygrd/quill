.. title:: Log Levels

Log Levels
==========

Quill offers two distinct mechanisms for controlling which log statements are processed and displayed: runtime filtering and compile-time filtering. Understanding both approaches is essential for optimizing logging performance in your application.

Runtime Log Level Control
-------------------------

The runtime log level is controlled through the ``Logger::set_log_level()`` function. This determines which log messages are actually processed during program execution.

Even if a log statement is compiled into your binary, it won't be displayed unless the logger's runtime log level is set appropriately. By default, loggers are configured with ``LogLevel::Info``, meaning that only INFO level messages and above (WARNING, ERROR, CRITICAL) will be displayed.

.. code-block:: cpp

   // Set the log level to display all log messages
   logger->set_log_level(quill::LogLevel::TraceL3);

The runtime log level can be adjusted dynamically during program execution, making it suitable for configuration via command-line arguments, configuration files, or programmatic adjustments during runtime.

Compile-Time Log Level Control
------------------------------

The compile-time log level is controlled through the ``QUILL_COMPILE_ACTIVE_LOG_LEVEL`` preprocessor definition. This determines the lowest log level that gets compiled into your binary.

Any log statements below the compile-time level are completely removed during compilation and become no-ops, with no runtime overhead. 

This provides an upper bound on what can be logged: if a log statement is compiled out, it can never be logged regardless of the runtime log level setting.

.. code-block:: cpp

   // Define before including LogMacros.h
   #define QUILL_COMPILE_ACTIVE_LOG_LEVEL QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO
   #include <quill/LogMacros.h>

   // Or set via CMake
   add_compile_definitions(-DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO)

If not explicitly defined, ``QUILL_COMPILE_ACTIVE_LOG_LEVEL`` defaults to ``QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3``, meaning all log levels are compiled in.

Compile-time filtering is primarily a late-stage optimization technique to completely eliminate lower-severity logs from the hot path, avoiding even the minimal overhead of the runtime check. However, this comes at the cost of flexibility, as changing the compile-time log level requires recompiling your application.

Interaction Between Runtime and Compile-Time Controls
-----------------------------------------------------

The two log level control mechanisms work together as follows:

1. The compile-time log level sets a hard upper limit on what can be logged
2. The runtime log level provides dynamic control within that limit

Available Log Levels
--------------------

Quill provides the following log levels, from most to least verbose:

- ``TRACE_L3``
- ``TRACE_L2``
- ``TRACE_L1``
- ``DEBUG``
- ``INFO``
- ``WARNING``
- ``ERROR``
- ``CRITICAL``

Each log level corresponds to a specific macro (e.g., ``LOG_INFO``, ``LOG_DEBUG``, etc.) that encodes the log level directly in the macro name.

In addition to the level-specific macros, Quill also provides the ``LOG_DYNAMIC`` macro that accepts the log level as a runtime parameter:

.. code-block:: cpp

   LOG_DYNAMIC(logger, quill::LogLevel::Info, "This log level is determined at runtime");

This is useful when the appropriate log level can only be determined during program execution, for example when passing the log level as a function argument or calculating it based on runtime conditions. Note that this flexibility comes with a minor performance cost compared to the static level macros.

