.. title:: Logging Macros

Logging Macros
==============

Choose ``LOG_*`` for a {fmt} message, ``LOGV_*`` for variable names and values, or ``LOGJ_*`` for named JSON
fields. Use ``LOG_DYNAMIC`` when severity is known only at runtime.

A macro evaluates the logger expression and any runtime level expression at most once. A compiled-out
macro evaluates neither. Disabled fixed-level macros skip their message arguments.

Named Placeholders
------------------

Quill supports named placeholders in format strings (e.g., ``{name}`` instead of ``{}``).
The library extracts the names from the format string and pairs them with the corresponding argument values.
This enables features such as structured JSON logging (see :doc:`JSON Logging <json_logging>`) and the
``%(named_args)`` attribute in pattern formatters (see :doc:`Formatters <formatters>`).

Arguments are always matched to placeholders **by position**, not by name lookup. The name inside
each ``{...}`` placeholder is used only as a label — it does not influence which argument fills it —
so arguments must be passed in the same order as their placeholders appear in the format string.
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

Compile-time filtering of log levels optimises performance by reducing branches in compiled code. Enable it by
defining `QUILL_COMPILE_ACTIVE_LOG_LEVEL` as a compilation flag or before including `LogMacros.h`:

.. code-block:: c

    QUILL_COMPILE_ACTIVE_LOG_LEVEL_{DESIRED_LEVEL}

`{DESIRED_LEVEL}` is one of `TRACE_L3`, `TRACE_L2`, `TRACE_L1`, `DEBUG`, `INFO`, `NOTICE`, `WARNING`, `ERROR`,
`CRITICAL`. In Quill's trace-tier naming, `TRACE_L3` is the most detailed trace level and `TRACE_L1` is the least
detailed trace level.

For example, to compile only with warnings and above, you would define:

.. code-block:: c

    #define QUILL_COMPILE_ACTIVE_LOG_LEVEL QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING
    #include "quill/LogMacros.h"

or

.. code-block:: cmake

    target_compile_definitions(${TARGET} PUBLIC QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)

:doc:`Log Levels <log_levels>` describes how this interacts with the runtime log level.

Standard Logging Macros
-----------------------

Every level has four macros:

- ``LOG_<LEVEL>(logger, fmt, ...)``
- ``LOG_<LEVEL>_LIMIT(min_interval, logger, fmt, ...)`` logs at most once every ``min_interval`` from
  its call site on each thread.
- ``LOG_<LEVEL>_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)`` logs once every ``n_occurrences`` calls
  of its call site on each thread.
- ``LOG_<LEVEL>_TAGS(logger, tags, fmt, ...)``

.. note::
    When using `_LIMIT` macros, you might need to manually `#include <chrono>` in your code. This header is not automatically included to avoid imposing unnecessary dependencies on all logging macros when only specific ones require it.

.. list-table::
   :header-rows: 1

   * - Level
     - Macro
     - ``_LIMIT``
     - ``_LIMIT_EVERY_N``
     - ``_TAGS``
   * - Trace Level 3 (L3)
     - ``LOG_TRACE_L3``
     - ``LOG_TRACE_L3_LIMIT``
     - ``LOG_TRACE_L3_LIMIT_EVERY_N``
     - ``LOG_TRACE_L3_TAGS``
   * - Trace Level 2 (L2)
     - ``LOG_TRACE_L2``
     - ``LOG_TRACE_L2_LIMIT``
     - ``LOG_TRACE_L2_LIMIT_EVERY_N``
     - ``LOG_TRACE_L2_TAGS``
   * - Trace Level 1 (L1)
     - ``LOG_TRACE_L1``
     - ``LOG_TRACE_L1_LIMIT``
     - ``LOG_TRACE_L1_LIMIT_EVERY_N``
     - ``LOG_TRACE_L1_TAGS``
   * - Debug
     - ``LOG_DEBUG``
     - ``LOG_DEBUG_LIMIT``
     - ``LOG_DEBUG_LIMIT_EVERY_N``
     - ``LOG_DEBUG_TAGS``
   * - Info
     - ``LOG_INFO``
     - ``LOG_INFO_LIMIT``
     - ``LOG_INFO_LIMIT_EVERY_N``
     - ``LOG_INFO_TAGS``
   * - Notice
     - ``LOG_NOTICE``
     - ``LOG_NOTICE_LIMIT``
     - ``LOG_NOTICE_LIMIT_EVERY_N``
     - ``LOG_NOTICE_TAGS``
   * - Warning
     - ``LOG_WARNING``
     - ``LOG_WARNING_LIMIT``
     - ``LOG_WARNING_LIMIT_EVERY_N``
     - ``LOG_WARNING_TAGS``
   * - Error
     - ``LOG_ERROR``
     - ``LOG_ERROR_LIMIT``
     - ``LOG_ERROR_LIMIT_EVERY_N``
     - ``LOG_ERROR_TAGS``
   * - Critical
     - ``LOG_CRITICAL``
     - ``LOG_CRITICAL_LIMIT``
     - ``LOG_CRITICAL_LIMIT_EVERY_N``
     - ``LOG_CRITICAL_TAGS``
   * - Backtrace
     - ``LOG_BACKTRACE``
     -
     -
     - ``LOG_BACKTRACE_TAGS``

Value-based Macros (LOGV)
-------------------------

These macros print the names and values of their arguments automatically, without naming each variable or
writing `{}` placeholders in the format string. Each macro can handle up to 26 arguments. The format string is
concatenated at compile time, so these macros have no runtime overhead.

- ``LOGV_<LEVEL>(logger, message, ...)``
- ``LOGV_<LEVEL>_LIMIT(min_interval, logger, message, ...)`` logs at most once every ``min_interval`` from
  its call site on each thread.
- ``LOGV_<LEVEL>_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`` logs once every ``n_occurrences`` calls
  of its call site on each thread.
- ``LOGV_<LEVEL>_TAGS(logger, tags, message, ...)``

.. list-table::
   :header-rows: 1

   * - Level
     - Macro
     - ``_LIMIT``
     - ``_LIMIT_EVERY_N``
     - ``_TAGS``
   * - Trace Level 3 (L3)
     - ``LOGV_TRACE_L3``
     - ``LOGV_TRACE_L3_LIMIT``
     - ``LOGV_TRACE_L3_LIMIT_EVERY_N``
     - ``LOGV_TRACE_L3_TAGS``
   * - Trace Level 2 (L2)
     - ``LOGV_TRACE_L2``
     - ``LOGV_TRACE_L2_LIMIT``
     - ``LOGV_TRACE_L2_LIMIT_EVERY_N``
     - ``LOGV_TRACE_L2_TAGS``
   * - Trace Level 1 (L1)
     - ``LOGV_TRACE_L1``
     - ``LOGV_TRACE_L1_LIMIT``
     - ``LOGV_TRACE_L1_LIMIT_EVERY_N``
     - ``LOGV_TRACE_L1_TAGS``
   * - Debug
     - ``LOGV_DEBUG``
     - ``LOGV_DEBUG_LIMIT``
     - ``LOGV_DEBUG_LIMIT_EVERY_N``
     - ``LOGV_DEBUG_TAGS``
   * - Info
     - ``LOGV_INFO``
     - ``LOGV_INFO_LIMIT``
     - ``LOGV_INFO_LIMIT_EVERY_N``
     - ``LOGV_INFO_TAGS``
   * - Notice
     - ``LOGV_NOTICE``
     - ``LOGV_NOTICE_LIMIT``
     - ``LOGV_NOTICE_LIMIT_EVERY_N``
     - ``LOGV_NOTICE_TAGS``
   * - Warning
     - ``LOGV_WARNING``
     - ``LOGV_WARNING_LIMIT``
     - ``LOGV_WARNING_LIMIT_EVERY_N``
     - ``LOGV_WARNING_TAGS``
   * - Error
     - ``LOGV_ERROR``
     - ``LOGV_ERROR_LIMIT``
     - ``LOGV_ERROR_LIMIT_EVERY_N``
     - ``LOGV_ERROR_TAGS``
   * - Critical
     - ``LOGV_CRITICAL``
     - ``LOGV_CRITICAL_LIMIT``
     - ``LOGV_CRITICAL_LIMIT_EVERY_N``
     - ``LOGV_CRITICAL_TAGS``
   * - Backtrace
     - ``LOGV_BACKTRACE``
     -
     -
     -

JSON Logging Macros (LOGJ)
--------------------------

These macros simplify JSON logging by embedding the name of each passed variable as a named argument in the
format string. Each macro can handle up to 26 arguments. The format string is concatenated at compile time, so
these macros have no runtime overhead.

- ``LOGJ_<LEVEL>(logger, message, ...)``
- ``LOGJ_<LEVEL>_LIMIT(min_interval, logger, message, ...)`` logs at most once every ``min_interval`` from
  its call site on each thread.
- ``LOGJ_<LEVEL>_LIMIT_EVERY_N(n_occurrences, logger, message, ...)`` logs once every ``n_occurrences`` calls
  of its call site on each thread.
- ``LOGJ_<LEVEL>_TAGS(logger, tags, message, ...)``

.. list-table::
   :header-rows: 1

   * - Level
     - Macro
     - ``_LIMIT``
     - ``_LIMIT_EVERY_N``
     - ``_TAGS``
   * - Trace Level 3 (L3)
     - ``LOGJ_TRACE_L3``
     - ``LOGJ_TRACE_L3_LIMIT``
     - ``LOGJ_TRACE_L3_LIMIT_EVERY_N``
     - ``LOGJ_TRACE_L3_TAGS``
   * - Trace Level 2 (L2)
     - ``LOGJ_TRACE_L2``
     - ``LOGJ_TRACE_L2_LIMIT``
     - ``LOGJ_TRACE_L2_LIMIT_EVERY_N``
     - ``LOGJ_TRACE_L2_TAGS``
   * - Trace Level 1 (L1)
     - ``LOGJ_TRACE_L1``
     - ``LOGJ_TRACE_L1_LIMIT``
     - ``LOGJ_TRACE_L1_LIMIT_EVERY_N``
     - ``LOGJ_TRACE_L1_TAGS``
   * - Debug
     - ``LOGJ_DEBUG``
     - ``LOGJ_DEBUG_LIMIT``
     - ``LOGJ_DEBUG_LIMIT_EVERY_N``
     - ``LOGJ_DEBUG_TAGS``
   * - Info
     - ``LOGJ_INFO``
     - ``LOGJ_INFO_LIMIT``
     - ``LOGJ_INFO_LIMIT_EVERY_N``
     - ``LOGJ_INFO_TAGS``
   * - Notice
     - ``LOGJ_NOTICE``
     - ``LOGJ_NOTICE_LIMIT``
     - ``LOGJ_NOTICE_LIMIT_EVERY_N``
     - ``LOGJ_NOTICE_TAGS``
   * - Warning
     - ``LOGJ_WARNING``
     - ``LOGJ_WARNING_LIMIT``
     - ``LOGJ_WARNING_LIMIT_EVERY_N``
     - ``LOGJ_WARNING_TAGS``
   * - Error
     - ``LOGJ_ERROR``
     - ``LOGJ_ERROR_LIMIT``
     - ``LOGJ_ERROR_LIMIT_EVERY_N``
     - ``LOGJ_ERROR_TAGS``
   * - Critical
     - ``LOGJ_CRITICAL``
     - ``LOGJ_CRITICAL_LIMIT``
     - ``LOGJ_CRITICAL_LIMIT_EVERY_N``
     - ``LOGJ_CRITICAL_TAGS``
   * - Backtrace
     - ``LOGJ_BACKTRACE``
     -
     -
     -

Dynamic Logging Macros
-----------------------

These macros take the log level at runtime, at a small cost compared with the fixed-level macros, which are
preferred for zero-cost logging.

- ``LOG_DYNAMIC(logger, log_level, fmt, ...)``
- ``LOG_DYNAMIC_TAGS(logger, log_level, tags, fmt, ...)``
- ``LOGV_DYNAMIC(logger, log_level, message, ...)``
- ``LOGJ_DYNAMIC(logger, log_level, message, ...)``

Runtime Metadata Logging Macro
------------------------------

Ordinary macros store call-site metadata at compile time. To supply it at runtime, choose a variant based
on which values can stay valid until the worker consumes the record.

- ``QUILL_LOG_RUNTIME_METADATA_DEEP`` copies format string, file, function and tags. Use it to
  forward logs from another library. ``QUILL_LOG_RUNTIME_METADATA`` is the same operation without tags.
- ``QUILL_LOG_RUNTIME_METADATA_HYBRID`` copies format and tags and references file and function.
  Tagged macro-free calls use this path.
- ``QUILL_LOG_RUNTIME_METADATA_SHALLOW`` references all four fields. It has the lowest overhead
  for static metadata with a dynamic level, as in ``LOG_DYNAMIC``.

``DEEP`` and ``HYBRID`` accept dynamic null-terminated tag strings. A ``SHALLOW`` tag pointer must stay
valid and immutable through backend consumption. :doc:`log_tagging` covers ``quill::Tags`` for runtime
``std::string`` and ``std::string_view`` inputs.

Binary Record Macro
-------------------

``LOG_BINARY(logger, data, size)`` writes raw bytes to the logger's sinks. It has no severity and bypasses
runtime and compile-time level filtering. See :doc:`binary_protocols`.
