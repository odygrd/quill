.. title:: Logging Macros

Logging Macros
==============

Complete reference for all ``LOG_``, ``LOGV_``, ``LOGJ_``, and ``LOG_DYNAMIC`` macros.

Named Placeholders
------------------

Quill supports named placeholders in format strings (e.g., ``{name}`` instead of ``{}``).
When named placeholders are used, the library extracts the names from the format string and pairs
them with the corresponding argument values. This enables features such as structured JSON logging
(see :doc:`JSON Logging <json_logging>`) and the ``%(named_args)`` attribute in pattern formatters
(see :doc:`Formatters <formatters>`).

Arguments are always matched to placeholders **by position**, not by name lookup. The name inside
each ``{...}`` placeholder is used only as a label — it does not influence which argument fills it.
This means arguments must be passed in the same order as their corresponding placeholders appear
in the format string.
If the same label appears more than once, later occurrences are suffixed (for example, ``id_1``)
in the structured ``named_args`` output so JSON fields remain distinct.

.. code-block:: cpp

   // Correct - arguments are passed in the same order as placeholders
   LOG_INFO(logger, "Hello {name}, you are {age}!", name, age);

   // LOGJ_ macros auto-generate the named format string from variable names
   LOGJ_INFO(logger, "Greeting", name, age);

.. note::

   ``fmtquill::arg()`` / ``fmt::arg()`` style named argument binding is **not supported**.
   Unlike ``fmtlib``, which resolves ``{name}`` placeholders by matching argument names at runtime,
   Quill serializes only the argument values (without names) to the queue to minimize frontend
   latency. Passing ``fmtquill::arg()`` / ``fmt::arg()`` to Quill logging calls will result in a
   compile error.

Compile-Time Log Level Filtering
--------------------------------

To optimise performance by reducing branches in compiled code, you can enable compile-time filtering of log levels.

This is done by defining `QUILL_COMPILE_ACTIVE_LOG_LEVEL` as a compilation flag or before including `LogMacros.h`:

.. code-block:: c

    QUILL_COMPILE_ACTIVE_LOG_LEVEL_{DESIRED_LEVEL}

Where `{DESIRED_LEVEL}` can be one of the following: `TRACE_L3`, `TRACE_L2`, `TRACE_L1`, `DEBUG`, `INFO`, `WARNING`, `ERROR`, `CRITICAL`.

In Quill's trace-tier naming, `TRACE_L3` is the most detailed trace level and `TRACE_L1` is the least detailed trace level.

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

- ``LOG_TRACE_L3(logger, fmt, ...)``

- ``LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_TRACE_L3_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_TRACE_L3_TAGS(logger, tags, fmt, ...)``

**Trace Level 2 (L2)**

- ``LOG_TRACE_L2(logger, fmt, ...)``

- ``LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_TRACE_L2_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_TRACE_L2_TAGS(logger, tags, fmt, ...)``

**Trace Level 1 (L1)**

- ``LOG_TRACE_L1(logger, fmt, ...)``

- ``LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_TRACE_L1_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_TRACE_L1_TAGS(logger, tags, fmt, ...)``

**Debug**

- ``LOG_DEBUG(logger, fmt, ...)``

- ``LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_DEBUG_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_DEBUG_TAGS(logger, tags, fmt, ...)``

**Info**

- ``LOG_INFO(logger, fmt, ...)``

- ``LOG_INFO_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_INFO_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_INFO_TAGS(logger, tags, fmt, ...)``

**Notice**

- ``LOG_NOTICE(logger, fmt, ...)``

- ``LOG_NOTICE_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_NOTICE_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_NOTICE_TAGS(logger, tags, fmt, ...)``

**Warning**

- ``LOG_WARNING(logger, fmt, ...)``

- ``LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_WARNING_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_WARNING_TAGS(logger, tags, fmt, ...)``

**Error**

- ``LOG_ERROR(logger, fmt, ...)``

- ``LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_ERROR_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_ERROR_TAGS(logger, tags, fmt, ...)``

**Critical**

- ``LOG_CRITICAL(logger, fmt, ...)``

- ``LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)``

- ``LOG_CRITICAL_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)``

- ``LOG_CRITICAL_TAGS(logger, tags, fmt, ...)``

**Backtrace**

- ``LOG_BACKTRACE(logger, fmt, ...)``

Value-based Macros (LOGV)
-------------------------

The following enhanced macros simplify logging by automatically printing variable names and values without explicitly specifying each variable name or using `{}` placeholders in the format string.
Each macro can handle up to 26 arguments. The format string is concatenated at compile time, there is no runtime overhead for using these macros.

**Trace Level 3 (L3)**

- ``LOGV_TRACE_L3(logger, message, ...)``

- ``LOGV_TRACE_L3_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_TRACE_L3_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_TRACE_L3_TAGS(logger, tags, message, ...)``

**Trace Level 2 (L2)**

- ``LOGV_TRACE_L2(logger, message, ...)``

- ``LOGV_TRACE_L2_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_TRACE_L2_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_TRACE_L2_TAGS(logger, tags, message, ...)``

**Trace Level 1 (L1)**

- ``LOGV_TRACE_L1(logger, message, ...)``

- ``LOGV_TRACE_L1_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_TRACE_L1_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_TRACE_L1_TAGS(logger, tags, message, ...)``

**Debug**

- ``LOGV_DEBUG(logger, message, ...)``

- ``LOGV_DEBUG_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_DEBUG_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_DEBUG_TAGS(logger, tags, message, ...)``

**Info**

- ``LOGV_INFO(logger, message, ...)``

- ``LOGV_INFO_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_INFO_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_INFO_TAGS(logger, tags, message, ...)``

**Notice**

- ``LOGV_NOTICE(logger, message, ...)``

- ``LOGV_NOTICE_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_NOTICE_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_NOTICE_TAGS(logger, tags, message, ...)``

**Warning**

- ``LOGV_WARNING(logger, message, ...)``

- ``LOGV_WARNING_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_WARNING_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_WARNING_TAGS(logger, tags, message, ...)``

**Error**

- ``LOGV_ERROR(logger, message, ...)``

- ``LOGV_ERROR_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_ERROR_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_ERROR_TAGS(logger, tags, message, ...)``

**Critical**

- ``LOGV_CRITICAL(logger, message, ...)``

- ``LOGV_CRITICAL_LIMIT(min_interval, logger, message, ...)``

- ``LOGV_CRITICAL_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGV_CRITICAL_TAGS(logger, tags, message, ...)``

**Backtrace**

- ``LOGV_BACKTRACE(logger, message, ...)``

JSON Logging Macros (LOGJ)
--------------------------

The following macros simplify JSON logging by automatically embedding the name of each passed variable as a named argument in the format string.
Each macro can handle up to 26 arguments. The format string is concatenated at compile time, there is no runtime overhead for using these macros.

**Trace Level 3 (L3)**

- ``LOGJ_TRACE_L3(logger, message, ...)``

- ``LOGJ_TRACE_L3_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_TRACE_L3_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_TRACE_L3_TAGS(logger, tags, message, ...)``

**Trace Level 2 (L2)**

- ``LOGJ_TRACE_L2(logger, message, ...)``

- ``LOGJ_TRACE_L2_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_TRACE_L2_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_TRACE_L2_TAGS(logger, tags, message, ...)``

**Trace Level 1 (L1)**

- ``LOGJ_TRACE_L1(logger, message, ...)``

- ``LOGJ_TRACE_L1_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_TRACE_L1_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_TRACE_L1_TAGS(logger, tags, message, ...)``

**Debug**

- ``LOGJ_DEBUG(logger, message, ...)``

- ``LOGJ_DEBUG_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_DEBUG_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_DEBUG_TAGS(logger, tags, message, ...)``

**Info**

- ``LOGJ_INFO(logger, message, ...)``

- ``LOGJ_INFO_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_INFO_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_INFO_TAGS(logger, tags, message, ...)``

**Notice**

- ``LOGJ_NOTICE(logger, message, ...)``

- ``LOGJ_NOTICE_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_NOTICE_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_NOTICE_TAGS(logger, tags, message, ...)``

**Warning**

- ``LOGJ_WARNING(logger, message, ...)``

- ``LOGJ_WARNING_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_WARNING_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_WARNING_TAGS(logger, tags, message, ...)``

**Error**

- ``LOGJ_ERROR(logger, message, ...)``

- ``LOGJ_ERROR_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_ERROR_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_ERROR_TAGS(logger, tags, message, ...)``

**Critical**

- ``LOGJ_CRITICAL(logger, message, ...)``

- ``LOGJ_CRITICAL_LIMIT(min_interval, logger, message, ...)``

- ``LOGJ_CRITICAL_LIMIT_EVERY_N(n_occurrences, logger, message, ...)``

- ``LOGJ_CRITICAL_TAGS(logger, tags, message, ...)``

**Backtrace**

- ``LOGJ_BACKTRACE(logger, message, ...)``

Dynamic Logging Macros
-----------------------

Dynamic logging macros provide runtime log level flexibility with a small overhead. Prefer using the compile-time log level macros for zero-cost logging.

- ``LOG_DYNAMIC(logger, log_level, fmt, ...)``

- ``LOGV_DYNAMIC(logger, log_level, message, ...)``

- ``LOGJ_DYNAMIC(logger, log_level, message, ...)``

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
