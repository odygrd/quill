.. title:: Logging Macros

Logging Macros
==============

Compile-Time Log Level Filtering
--------------------------------

To optimise performance by reducing branches in compiled code, you can enable compile-time filtering of log levels.

This is done by defining `QUILL_COMPILE_ACTIVE_LOG_LEVEL` as a compilation flag or before including `LogMacros.h`:

.. code-block:: c

    QUILL_COMPILE_ACTIVE_LOG_LEVEL_{DESIRED_LEVEL}

Where `{DESIRED_LEVEL}` can be one of the following: `TRACE_L3`, `TRACE_L2`, `TRACE_L1`, `DEBUG`, `INFO`, `WARNING`, `ERROR`, `CRITICAL`.

For example, to compile only with warnings and above, you would define:

.. code-block:: c

    #define QUILL_COMPILE_ACTIVE_LOG_LEVEL QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING
    #include "quill/LogMacros.h"

or

.. code-block:: cmake

    target_compile_definitions(${TARGET} PUBLIC -DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)

.. note::
    When using `_LIMIT` macros, you might need to manually `#include <chrono>` in your code. This header is not automatically included to avoid imposing unnecessary dependencies on all logging macros when only specific ones require it.

Standard Logging Macros
-----------------------

**Trace Level 3 (L3)**

- :c:macro:`LOG_TRACE_L3(logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L3_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L3_TAGS(logger, tags, fmt, ...)`

**Trace Level 2 (L2)**

- :c:macro:`LOG_TRACE_L2(logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L2_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L2_TAGS(logger, tags, fmt, ...)`

**Trace Level 1 (L1)**

- :c:macro:`LOG_TRACE_L1(logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L1_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_TRACE_L1_TAGS(logger, tags, fmt, ...)`

**Debug**

- :c:macro:`LOG_DEBUG(logger, fmt, ...)`

- :c:macro:`LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_DEBUG_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_DEBUG_TAGS(logger, tags, fmt, ...)`

**Info**

- :c:macro:`LOG_INFO(logger, fmt, ...)`

- :c:macro:`LOG_INFO_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_INFO_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_INFO_TAGS(logger, tags, fmt, ...)`

**Notice**

- :c:macro:`LOG_NOTICE(logger, fmt, ...)`

- :c:macro:`LOG_NOTICE_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_NOTICE_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_NOTICE_TAGS(logger, tags, fmt, ...)`

**Warning**

- :c:macro:`LOG_WARNING(logger, fmt, ...)`

- :c:macro:`LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_WARNING_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_WARNING_TAGS(logger, tags, fmt, ...)`

**Error**

- :c:macro:`LOG_ERROR(logger, fmt, ...)`

- :c:macro:`LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_ERROR_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_ERROR_TAGS(logger, tags, fmt, ...)`

**Critical**

- :c:macro:`LOG_CRITICAL(logger, fmt, ...)`

- :c:macro:`LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)`

- :c:macro:`LOG_CRITICAL_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`

- :c:macro:`LOG_CRITICAL_TAGS(logger, tags, fmt, ...)`

**Backtrace**

- :c:macro:`LOG_BACKTRACE(logger, fmt, ...)`

Value-based Macros (LOGV)
-------------------------

The following enhanced macros simplify logging by automatically printing variable names and values without explicitly specifying each variable name or using `{}` placeholders in the format string.
Each macro can handle up to 26 arguments. The format string is concatenated at compile time, there is no runtime overhead for using these macros.

**Trace Level 3 (L3)**

- :c:macro:`LOGV_TRACE_L3(logger, message, ...)`

- :c:macro:`LOGV_TRACE_L3_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_TRACE_L3_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_TRACE_L3_TAGS(logger, tags, message, ...)`

**Trace Level 2 (L2)**

- :c:macro:`LOGV_TRACE_L2(logger, message, ...)`

- :c:macro:`LOGV_TRACE_L2_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_TRACE_L2_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_TRACE_L2_TAGS(logger, tags, message, ...)`

**Trace Level 1 (L1)**

- :c:macro:`LOGV_TRACE_L1(logger, message, ...)`

- :c:macro:`LOGV_TRACE_L1_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_TRACE_L1_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_TRACE_L1_TAGS(logger, tags, message, ...)`

**Debug**

- :c:macro:`LOGV_DEBUG(logger, message, ...)`

- :c:macro:`LOGV_DEBUG_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_DEBUG_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_DEBUG_TAGS(logger, tags, message, ...)`

**Info**

- :c:macro:`LOGV_INFO(logger, message, ...)`

- :c:macro:`LOGV_INFO_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_INFO_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_INFO_TAGS(logger, tags, message, ...)`

**Notice**

- :c:macro:`LOGV_NOTICE(logger, message, ...)`

- :c:macro:`LOGV_NOTICE_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_NOTICE_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_NOTICE_TAGS(logger, tags, message, ...)`

**Warning**

- :c:macro:`LOGV_WARNING(logger, message, ...)`

- :c:macro:`LOGV_WARNING_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_WARNING_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_WARNING_TAGS(logger, tags, message, ...)`

**Error**

- :c:macro:`LOGV_ERROR(logger, message, ...)`

- :c:macro:`LOGV_ERROR_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_ERROR_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_ERROR_TAGS(logger, tags, message, ...)`

**Critical**

- :c:macro:`LOGV_CRITICAL(logger, message, ...)`

- :c:macro:`LOGV_CRITICAL_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGV_CRITICAL_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGV_CRITICAL_TAGS(logger, tags, message, ...)`

**Backtrace**

- :c:macro:`LOGV_BACKTRACE(logger, message, ...)`

JSON Logging Macros (LOGJ)
--------------------------

The following macros simplify JSON logging by automatically embedding the name of each passed variable as a named argument in the format string.
Each macro can handle up to 26 arguments. The format string is concatenated at compile time, there is no runtime overhead for using these macros.

**Trace Level 3 (L3)**

- :c:macro:`LOGJ_TRACE_L3(logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L3_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L3_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L3_TAGS(logger, tags, message, ...)`

**Trace Level 2 (L2)**

- :c:macro:`LOGJ_TRACE_L2(logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L2_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L2_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L2_TAGS(logger, tags, message, ...)`

**Trace Level 1 (L1)**

- :c:macro:`LOGJ_TRACE_L1(logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L1_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L1_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_TRACE_L1_TAGS(logger, tags, message, ...)`

**Debug**

- :c:macro:`LOGJ_DEBUG(logger, message, ...)`

- :c:macro:`LOGJ_DEBUG_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_DEBUG_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_DEBUG_TAGS(logger, tags, message, ...)`

**Info**

- :c:macro:`LOGJ_INFO(logger, message, ...)`

- :c:macro:`LOGJ_INFO_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_INFO_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_INFO_TAGS(logger, tags, message, ...)`

**Notice**

- :c:macro:`LOGJ_NOTICE(logger, message, ...)`

- :c:macro:`LOGJ_NOTICE_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_NOTICE_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_NOTICE_TAGS(logger, tags, message, ...)`

**Warning**

- :c:macro:`LOGJ_WARNING(logger, message, ...)`

- :c:macro:`LOGJ_WARNING_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_WARNING_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_WARNING_TAGS(logger, tags, message, ...)`

**Error**

- :c:macro:`LOGJ_ERROR(logger, message, ...)`

- :c:macro:`LOGJ_ERROR_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_ERROR_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_ERROR_TAGS(logger, tags, message, ...)`

**Critical**

- :c:macro:`LOGJ_CRITICAL(logger, message, ...)`

- :c:macro:`LOGJ_CRITICAL_LIMIT(min_interval, logger, message, ...)`

- :c:macro:`LOGJ_CRITICAL_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`

- :c:macro:`LOGJ_CRITICAL_TAGS(logger, tags, message, ...)`

**Backtrace**

- :c:macro:`LOGJ_BACKTRACE(logger, message, ...)`

Dynamic Logging Macros
-----------------------

Dynamic logging macros provide runtime log level flexibility with a small overhead. Prefer using the compile-time log level macros for zero-cost logging.

- :c:macro:`LOG_DYNAMIC(logger, log_level, fmt, ...)`

- :c:macro:`LOGV_DYNAMIC(logger, log_level, message, ...)`

- :c:macro:`LOGJ_DYNAMIC(logger, log_level, message, ...)`

Runtime Metadata Logging Macro
------------------------------

By default, the library creates and stores metadata information (e.g., source location) for each log statement at compile time.
It is also possible to supply this metadata at runtime along with a log message. While this provides runtime flexibility,
it introduces some overhead compared to compile-time metadata macros. Therefore, it is recommended to prefer using
the compile-time metadata macros whenever possible. Quill provides three specialized macros for working with runtime metadata, each offering different trade-offs between flexibility and performance:

- :c:macro:`QUILL_LOG_RUNTIME_METADATA_DEEP` - Takes a deep copy of ``fmt``, ``file``, ``function`` and ``tags``. Most flexible option, suitable for forwarding logs from another logging library to Quill.

- :c:macro:`QUILL_LOG_RUNTIME_METADATA_HYBRID` - Takes a deep copy of ``fmt`` and ``tags``, while referencing ``file`` and ``function``. Used for the macro-free mode.

- :c:macro:`QUILL_LOG_RUNTIME_METADATA_SHALLOW` - Takes everything as reference. Most efficient option when using compile-time metadata with dynamic log levels like ``LOG_DYNAMIC``.

Note that ``QUILL_LOG_RUNTIME_METADATA`` is equivalent to ``QUILL_LOG_RUNTIME_METADATA_DEEP`` but without the ``tags`` parameter.