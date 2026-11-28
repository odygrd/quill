.. title:: Log Levels

Log Levels
==========

Set a runtime threshold for each logger, or compile low-severity macros out of the application when their
level will never be needed.

Runtime Log Level Control
-------------------------

``Logger::set_log_level()`` sets the lowest severity the logger processes. The default, ``LogLevel::Info``,
admits Info, Notice, Warning, Error and Critical.

.. code-block:: cpp

   // Set the log level to display all log messages
   logger->set_log_level(quill::LogLevel::TraceL3);

Change the threshold at any time, for example from command-line arguments, a configuration file or application code.

Setting the Log Level from the Environment
-------------------------------------------

``QUILL_LOG_LEVEL`` overrides the default at logger creation. Existing loggers keep their levels. You can
still change them with ``set_log_level()``.

The accepted, case-insensitive values are ``tracel3``/``trace_l3``, ``tracel2``/``trace_l2``, ``tracel1``/``trace_l1``, ``debug``,
``info``, ``notice``, ``warning``/``warn``, ``error``/``err``, ``critical`` and ``none``. The internal ``backtrace`` level
is rejected.

.. code-block:: shell

   QUILL_LOG_LEVEL=debug ./my_application

An invalid value throws ``QuillError`` naming the variable when a logger is created. Creation registers
nothing and can be retried after correcting the environment. ``QUILL_NO_EXCEPTIONS`` builds take the fatal
error path.

Compile-Time Log Level Control
------------------------------

``QUILL_COMPILE_ACTIVE_LOG_LEVEL`` removes level-specific macros below the selected severity, including their
argument evaluation and runtime level checks.

.. code-block:: cpp

   // Define before including LogMacros.h
   #define QUILL_COMPILE_ACTIVE_LOG_LEVEL QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO
   #include <quill/LogMacros.h>

   // Or set via CMake
   add_compile_definitions(-DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO)

The default ``QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3`` includes all levels. Rebuild to change the cutoff.
See :ref:`logging_macros:compile-time log level filtering` for compiler flags.

Interaction Between Runtime and Compile-Time Controls
-----------------------------------------------------

A runtime threshold can narrow the levels compiled in. Lowering it cannot restore an omitted macro.
``LOG_DYNAMIC`` and :doc:`logging functions <macro_free_mode>` keep their runtime checks.

Available Log Levels
--------------------

The levels, from most to least verbose, are ``TRACE_L3``, ``TRACE_L2``, ``TRACE_L1``, ``DEBUG``, ``INFO``,
``NOTICE``, ``WARNING``, ``ERROR`` and ``CRITICAL``. Quill's trace numbering makes L3 the most detailed.

Level-specific macros encode severity in their name, e.g. ``LOG_DEBUG`` or ``LOG_INFO``. Use ``LOG_DYNAMIC``
when the level comes from a function argument or a runtime calculation.

.. code-block:: cpp

   LOG_DYNAMIC(logger, quill::LogLevel::Info, "This log level is determined at runtime");

Selecting severity at runtime adds a small cost compared with a fixed-level macro.
