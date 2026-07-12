.. title:: Macro-Free Mode

Macro-Free Mode
===============

The library provides a macro-free mode that allows logging using functions instead of macros. While the macro mode remains the main and recommended approach for logging, the macro-free mode offers an alternative with cleaner code in some scenarios, at the cost of additional overhead.

The macro-free mode is implemented using compiler built-ins (``__builtin_FILE()``, ``__builtin_FUNCTION()``, ``__builtin_LINE()``) which may vary by compiler.

Performance Trade-offs
----------------------

The macro-free approach comes with performance implications:

1. **Higher Latency**: Format metadata isn't available at compile time, requiring additional runtime copying of metadata to the backend thread

2. **Always-Evaluated Arguments**: Unlike macros which skip evaluation for disabled log levels, arguments to these functions are always evaluated

3. **No Compile-Time Removal**: Cannot be completely compiled out with ``QUILL_COMPILE_ACTIVE_LOG_LEVEL_<LEVEL>`` as macros can

4. **Backend Thread Impact**: Reduced throughput in the backend due to runtime metadata storage and processing

5. **Additional Safety Checks**: The macro-free functions perform a runtime check for `nullptr` logger. This can be useful in cases where a user might forget to initialize a logger or intentionally wants logging calls to become no-ops when the logger is null. (Note that there are also multiple ways to achieve no-op logging with macros, e.g., by setting the log level to `None`)

For performance-critical logging paths, the macro-based logging interface is recommended.

Available Functions
-------------------

The following logging functions are available in the macro-free mode:

- ``quill::tracel3(logger, fmt, args...)`` - Trace level 3 (most detailed)
- ``quill::tracel2(logger, fmt, args...)`` - Trace level 2
- ``quill::tracel1(logger, fmt, args...)`` - Trace level 1
- ``quill::debug(logger, fmt, args...)`` - Debug level
- ``quill::info(logger, fmt, args...)`` - Information level
- ``quill::notice(logger, fmt, args...)`` - Notice level
- ``quill::warning(logger, fmt, args...)`` - Warning level
- ``quill::error(logger, fmt, args...)`` - Error level
- ``quill::critical(logger, fmt, args...)`` - Critical level
- ``quill::backtrace(logger, fmt, args...)`` - Backtrace level

Each function also accepts a ``quill::Tags`` object as an optional parameter after the logger.

Named placeholders (e.g., ``{name}`` instead of ``{}``) work the same way as with ``LOG_`` macros.
See :ref:`Named Placeholders <logging_macros:Named Placeholders>` for details.

.. note:: Rate-limited logging (``LOG_<LEVEL>_LIMIT`` and ``LOG_<LEVEL>_LIMIT_EVERY_N``) has no
   macro-free equivalent and is out of scope for this mode. The limit macros maintain throttling
   state per call site through ``static thread_local`` variables declared inside the macro
   expansion; a function API has no call-site identity, so every call would share the same state.
   Use the ``LOG_<LEVEL>_LIMIT`` macros when throttled logging is needed.

Usage
-----

.. literalinclude:: ../examples/console_logging_macro_free.cpp
   :language: cpp
   :linenos:
