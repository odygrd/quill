.. title:: Timestamp Types

Timestamp Types
===============

Use this page to choose between TSC, system, or custom clock sources and understand their trade-offs.

Timestamp Recording
-------------------

The caller thread timestamps each record just before publishing it to its queue. Time spent waiting for queue space,
formatting an argument on the frontend, or copying a large payload therefore falls before the timestamp.

A thread can still be preempted between reading the clock and publishing the record.
:cpp:member:`BackendOptions::log_timestamp_ordering_grace_period` delays the backend by a short window (5 μs by default)
to allow older records from other threads to arrive.
A queue head that stays ahead of the wall-clock cutoff becomes eligible once one grace period of steady time has elapsed since its first backend observation. Backward clock corrections therefore do not require waiting for wall time to catch up.
The backend assumes timestamps produced within a single frontend queue are normally non-decreasing.
If a wall clock or user-provided clock moves backwards, :cpp:member:`BackendOptions::ensure_monotonic_output_timestamps` can be enabled to correct regular log, metric and binary record output.
Backtrace records preserve their original capture timestamps and may still appear out of timestamp order when flushed later.

If a regular log, metric or binary record has an older timestamp than one already selected for output,
the backend can suggest a larger grace period through :cpp:member:`BackendOptions::error_notifier`:

.. code-block:: text

   Quill WARNING: A record is 571 us out of timestamp order. For delayed publishers, consider increasing BackendOptions::log_timestamp_ordering_grace_period to at least 600 us.

The backend adds the observed timestamp difference in whole microseconds to the configured grace period and rounds up to one significant digit.
It reports again only when the estimate exceeds the last suggestion. A larger grace period can help a delayed publisher, but cannot correct a clock that moves backwards.
The notification is disabled when the grace period is zero or monotonic output correction is enabled.
Custom epoch timestamps are excluded from the comparison, so simulation loggers can use their own timeline alongside wall-clock loggers.

Timestamp Methods
-----------------

There are several ways to get the timestamp on the frontend:

- **TSC (Time Stamp Counter)**: This is the fastest method for the frontend, as it simply calls ``__rdtsc`` and pushes the raw TSC counter value. The backend runs an ``RdtscClock`` which periodically syncs with the wall time (default every 500ms). It takes the ``rdtsc`` value from the frontend and converts it to wall time using a lock-free algorithm. \
  Although the ``rdtsc`` clock might drift between resyncs, the difference is generally negligible. However, your processor must support invariant TSC to ensure that TSC counters between different cores remain synchronized. When this timestamp type is used, the backend takes about 10 ms for the initial calibration of the ``RdtscClock`` on first usage.

  **Note**: TSC timestamps in rare cases may appear out of chronological order by a few microseconds. This occurs because: (1) frontend threads on different CPU cores capture raw TSC values with potential slight variations, (2) the backend converts these TSC values to wall time using a periodically updated calibration point, and (3) when resync occurs between message captures, different messages may be converted using different calibration points. Since the RdtscClock is not monotonic across resyncs, timestamps from different cores converted with different calibrations can appear out of order even when the actual logging occurred in sequence. This is an inherent characteristic of TSC-based timing across multi-core systems. If strict chronological timestamp ordering is required, use the System clock method instead.

- **System**: This method calls ``std::chrono::system_clock::now()`` on the frontend and pushes the wall time value to the queue. It is slower than TSC but provides the most accurate and immediately usable timestamps. The backend thread does not require any additional initialization time since the ``RdtscClock`` is not used.

- **User**: Pass a ``UserClockSource`` object to provide custom timestamps. Its optional virtual
  ``timestamp_type() const noexcept`` method reports the return format: ``TimestampType::TscTicks``
  or ``TimestampType::EpochNanoseconds`` (the default).

Pass one clock configuration when creating a logger: ``ClockSource::tsc()``,
``ClockSource::system()``, or ``ClockSource::custom(clock)``. Omitting it selects TSC. With the
default frontend, these choices share the same ``Logger*`` type, so one binary can choose live
or simulation clocks through runtime configuration.
With default frontend options, custom clocks use a virtual ``UserClockSource::now()`` call.
The backend uses ``timestamp_type()`` to interpret custom clock values.

Providing a Custom Timestamp
----------------------------

.. literalinclude:: ../examples/user_clock_source.cpp
   :language: cpp
   :linenos:

Frontend Clock Selection
------------------------

``FrontendOptions::clock_type`` selects how the frontend reads timestamps. Its default,
``quill::ClockSource``, is the runtime configuration type accepted by logger creation. A
``ClockSource`` variable can hold any of the three selections. Options types without
``clock_type`` retain this runtime behavior.

To inline a clock, set ``clock_type`` to the ``UserClockSource`` subclass implementing the
``now() const noexcept`` function you want called, and pass ``ClockSource::custom(clock)`` through the same
factory argument. This expression retains the clock's type for compile-time checking.
The frontend calls that implementation directly, without virtual dispatch. The selected type may
be abstract if it already implements ``now()``; pass an instance of a derived class in that case.
Further overrides of ``now()`` in derived classes are bypassed; the backend still calls
``timestamp_type()`` virtually, so it must describe the selected ``now()`` implementation.
The option selects dispatch; the argument supplies the clock object and its state.

``ClockSource`` selects a clock implementation; ``TimestampType`` describes the value returned by
``now()``. The clock object's virtual ``timestamp_type() const noexcept`` defaults to
``TimestampType::EpochNanoseconds`` for compatibility with existing clocks:

- ``TimestampType::TscTicks`` means raw TSC ticks, converted to epoch nanoseconds by the backend.
- ``TimestampType::EpochNanoseconds`` means nanoseconds since the Unix epoch. Custom clocks retain
  the existing authoritative user-clock ordering behavior, whether the value is simulation time
  or a wall-clock reading.

The clock is non-owned and must outlive its logger. Both clock methods must be thread-safe, and
its timestamp representation must remain consistent for the logger's lifetime, including queued
records. Different loggers may use different instances of the configured clock type. The clock
is read once for each record that is queued, and not for a record that is dropped. Zero is a
valid nanosecond timestamp.

Declare custom overrides as ``uint64_t now() const noexcept override``. They must return normally and must not log through Quill,
including through another logger. The calling thread has already encoded the record in its queue.
A nested log could overwrite it, and an exception could leave a deferred argument in the uncommitted record.

``ClockSource::custom(clock)`` is non-owning and accepts a clock object by reference. When
``clock_type`` names an implementation class, this reference must refer to that class or a
derived class. Passing a different clock type, selecting a built-in source, or omitting the
clock argument when creating a logger is a compile-time error. Use one frontend/options type
throughout the application and its shared libraries, including typed signal-handler setup.

The same factory expression works with the default frontend (virtual dispatch) and a frontend
configured with the clock's implementation type (direct dispatch):

.. literalinclude:: snippets/quill_docs_custom_clock.cpp
   :language: cpp
   :linenos:

If every logger in the application uses TSC, an instance of the following clock removes runtime
clock selection. It uses the same inline TSC read and queue encoder as the default TSC logger
and can save a few frontend instructions, depending on the compiler and build options.

.. literalinclude:: snippets/quill_docs_custom_tsc_clock.cpp
   :language: cpp
   :start-at: struct TscClock

Updating Existing Logger Creation Calls
---------------------------------------

Replace the previous enum/pointer pair with one clock selection. Default TSC creation calls
without clock arguments remain the same:

.. code-block:: cpp

   // Previous: ..., pattern_options, ClockSourceType::User, &clock
   // Current:
   Frontend::create_or_get_logger("app", sink, pattern_options, ClockSource::custom(clock));

Use ``ClockSource::tsc()`` or ``ClockSource::system()`` for explicit built-in clock selections.

Getting a Synchronized Timestamp with the Backend Thread TSC Clock
------------------------------------------------------------------

In some cases, when using TSC for log statements, you might want to obtain a timestamp that is synchronized with the timestamp seen in the log statements.
To achieve this, you can use the :cpp:class:`BackendTscClock`. See the example below:

.. literalinclude:: snippets/quill_docs_timestamp_types.cpp
   :language: cpp
   :linenos:
