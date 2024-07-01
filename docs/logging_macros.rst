Logging Macros
==============

The following macros are provided for logging.

To optimize performance by reducing branches in compiled code, you can enable compile-time filtering of log levels.

This is done by defining `QUILL_COMPILE_ACTIVE_LOG_LEVEL` as a compilation flag or before including `LogMacros.h`:

.. code-block:: c

    QUILL_COMPILE_ACTIVE_LOG_LEVEL_{DESIRED_LEVEL}

Where `{DESIRED_LEVEL}` can be one of the following: `TRACE_L3`, `TRACE_L2`, `TRACE_L1`, `DEBUG`, `INFO`, `WARNING`, `ERROR`, `CRITICAL`.

For example, to compile with warnings and above, you would define:

.. code-block:: c

    #define QUILL_COMPILE_ACTIVE_LOG_LEVEL QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING
    #include "quill/LogMacros.h"

or

.. code-block:: cmake

    target_compile_definitions(${TARGET} PUBLIC -DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)

Standard Logging Macros
-----------------------

**Trace Level 3 (L3)**

- :c:macro:`LOG_TRACE_L3(logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ...)`

**Trace Level 2 (L2)**

- :c:macro:`LOG_TRACE_L2(logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ...)`

**Trace Level 1 (L1)**

- :c:macro:`LOG_TRACE_L1(logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ...)`

**Debug**

- :c:macro:`LOG_DEBUG(logger, fmt, ...)`

- :c:macro:`LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ...)`

**Info**

- :c:macro:`LOG_INFO(logger, fmt, ...)`

- :c:macro:`LOG_INFO_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_INFO_WITH_TAGS(logger, tags, fmt, ...)`

**Warning**

- :c:macro:`LOG_WARNING(logger, fmt, ...)`

- :c:macro:`LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_WARNING_WITH_TAGS(logger, tags, fmt, ...)`

**Error**

- :c:macro:`LOG_ERROR(logger, fmt, ...)`

- :c:macro:`LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_ERROR_WITH_TAGS(logger, tags, fmt, ...)`

**Critical**

- :c:macro:`LOG_CRITICAL(logger, fmt, ...)`

- :c:macro:`LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ...)`

**Backtrace**

- :c:macro:`LOG_BACKTRACE(logger, fmt, ...)`

Dynamic Logging Macros
-----------------------

Dynamic logging macros provide runtime log level flexibility with a small overhead. Prefer using the compile-time log level macros for zero-cost logging.

- :c:macro:`LOG_DYNAMIC(logger, log_level, fmt, ...)`