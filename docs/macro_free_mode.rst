.. title:: Macro-Free Mode

Macro-Free Mode
===============

Use ``quill::info()`` and related functions for ordinary function-call syntax. Prefer :doc:`logging_macros`
where disabled arguments or the lowest caller latency matter. Functions capture source location with
compiler-dependent built-ins (``__builtin_FILE()``, ``__builtin_FUNCTION()``, ``__builtin_LINE()``).

Performance Trade-offs
----------------------

**Close to the macros in speed:** with C++20, a call whose format string is a string literal and that has no
tags. The format string and the source location of the call are measured and scanned at compile time, and the
record refers to them instead of copying them. As with the macros, the format string is not validated at compile
time, see :doc:`FAQ <faq>`.

**Slower:** the calls that copy the format string and the tags into the queue each time instead, which costs
more on the calling thread and on the backend:

- a format string that is not a literal,
- a call with tags,
- every call in a C++17 build,
- call sites whose format string is longer than 65,535 bytes, whose line is above 1,048,575, whose file path is
  longer than 16,383 bytes, whose file name starts more than 4,095 bytes into its path, or whose file name
  contains a ``:``.

With C++20, a ``const char[N]`` format string must be a string literal or a ``constexpr`` array with
static storage, such as a ``static constexpr`` one. Pass ``+fmt`` or a pointer otherwise, e.g. for a
local array, a ``char const (&)[N]`` function parameter or a conditional expression that selects one
of two literals of the same length. The format string is then copied, as the backend reads a literal
after the call returns.

The functions also differ from the macros in these ways:

1. **Always-evaluated arguments:** macros skip evaluation for disabled log levels; function arguments are always
   evaluated, so a disabled call still pays for an argument that is expensive to compute, as the expensive
   argument table of the :ref:`disabled statement benchmark <benchmarks_latency:disabled statement>` shows. The
   call also checks the logger for null and, with some compilers, builds its format string object before it checks
   the log level, so even with cheap arguments a disabled call costs a few more instructions than a disabled macro.
2. **No compile-time removal:** ``QUILL_COMPILE_ACTIVE_LOG_LEVEL_<LEVEL>`` cannot compile the functions out, as it
   does the macros.
3. **Null-logger check:** a null logger makes the call a no-op, useful when a logger may be uninitialized. (Macros
   become no-ops in other ways, e.g. with the log level set to ``None``.)

For performance-critical logging paths, the macros are recommended.

Available Functions
-------------------

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

``quill::Tags`` owns its tag text, so runtime-selected tags are safe to pass through the macro-free API: see
:ref:`Dynamic Tags <log_tagging:Dynamic Tags>` for the accepted inputs and their lifetime, and
:doc:`Log Tagging <log_tagging>` for compile-time alternatives and backend tag processing.

Functions support the same :ref:`named placeholders <logging_macros:named placeholders>` as macros.

.. note:: Rate-limited logging (``LOG_<LEVEL>_LIMIT`` and ``LOG_<LEVEL>_LIMIT_EVERY_N``) has no function
   equivalent: the limit macros keep throttling state per call site in ``static thread_local`` variables declared in
   the macro expansion, and a function has no call-site identity, so every call would share one state. Use the
   macros for throttled logging.

Usage
-----

.. literalinclude:: ../examples/console_logging_macro_free.cpp
   :language: cpp
   :linenos:
