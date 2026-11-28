.. title:: Macro-Free Mode

Macro-Free Mode
===============

The library provides a macro-free mode that allows logging using functions instead of macros. While the macro mode remains the main and recommended approach for logging, the macro-free mode offers an alternative with cleaner code in some scenarios, at the cost of additional overhead.

The macro-free mode is implemented using compiler built-ins (``__builtin_FILE()``, ``__builtin_FUNCTION()``, ``__builtin_LINE()``) which may vary by compiler.

Performance Trade-offs
----------------------

With C++20, a call whose format string is a string literal and that has no tags is close to the
macros in speed: the format string and the source location of the call are measured and scanned at
compile time, and the record refers to them instead of copying them. As with the macros, the format
string is not validated at compile time, see :doc:`FAQ <faq>`. A format string that is not a
literal, a call with tags, and every call in a C++17 build copy the format string and the tags into
the queue on each call instead, which costs more on the calling thread and on the backend. So do
call sites whose format string is longer than 65,535 bytes, whose line is above 1,048,575, whose
file path is longer than 16,383 bytes, whose file name starts more than 4,095 bytes into its path,
or whose file name contains a ``:``.

With C++20, a ``const char[N]`` format string must be a string literal or a ``constexpr`` array with
static storage, such as a ``static constexpr`` one. Pass ``+fmt`` or a pointer otherwise, e.g. for a
local array, a ``char const (&)[N]`` function parameter or a conditional expression that selects one
of two literals of the same length. The format string is then copied, as the backend reads a literal
after the call returns.

The macro-free approach also differs from the macros in these ways:

1. **Always-Evaluated Arguments**: Unlike macros which skip evaluation for disabled log levels, arguments to these functions are always evaluated. A disabled call still pays for an argument that is expensive to compute, as the expensive argument table of the :ref:`disabled statement benchmark <benchmarks_latency:disabled statement>` shows. The call also checks the logger for null and, with some compilers, builds its format string object before it checks the log level, so with cheap arguments a statement whose level is disabled costs a few more instructions than a disabled macro

2. **No Compile-Time Removal**: Cannot be completely compiled out with ``QUILL_COMPILE_ACTIVE_LOG_LEVEL_<LEVEL>`` as macros can

3. **Additional Safety Checks**: The macro-free functions perform a runtime check for `nullptr` logger. This can be useful in cases where a user might forget to initialize a logger or intentionally wants logging calls to become no-ops when the logger is null. (Note that there are also multiple ways to achieve no-op logging with macros, e.g., by setting the log level to `None`)

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
- ``quill::log(logger, log_level, fmt, args...)`` - Level selected at runtime

The level-specific functions also accept a ``quill::Tags`` object as an optional parameter after
the logger. For ``quill::log``, pass the tags after the runtime log level:

.. code-block:: cpp

   quill::log(logger, log_level, quill::Tags{"NETWORK"}, "Connected to {}", endpoint);

``quill::Tags`` owns its combined tag text and accepts ``char const*``, ``std::string``, and
``std::string_view`` inputs. This makes runtime-selected tags safe to pass through the macro-free
API, including string views that are not null-terminated. Macro-free logging copies the resulting
tags into the asynchronous queue before the call returns.

See :doc:`Log Tagging <log_tagging>` for compile-time alternatives and backend tag processing.

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
