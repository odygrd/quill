.. title:: Timestamp Types

Timestamp Types
===============

Choose a clock when creating a logger. TSC minimizes caller overhead, the system clock records wall time
directly, and a custom clock can supply hardware, simulation or replay time.

Timestamp Recording
-------------------

The caller timestamps each record just before publishing, after waiting for space, frontend formatting and
copying the payload. Preemption between reading the clock and publishing can still delay a record.

- :cpp:member:`BackendOptions::log_timestamp_ordering_grace_period` (default 5 µs) lets earlier timestamps
  arrive from other threads. If a queue head stays ahead of the wall-clock cutoff, one grace period of
  steady time from its first observation makes it eligible even after a backward clock adjustment.
- The backend writes eligible records from other queues while waiting. If none are eligible, it waits the
  shorter of the grace period and :cpp:member:`BackendOptions::sleep_duration`. A zero sleep polls immediately.
- ``Backend::stop()`` drains without the grace period. User clocks returning ``TimestampType::EpochNanoseconds``
  also bypass it.

Timestamps in one queue should normally be non-decreasing. :cpp:member:`BackendOptions::ensure_monotonic_output_timestamps`
corrects sink-visible times for regular log, metric and binary records if a clock moves backward.
Flushed backtraces keep their original capture times.

If a regular log, metric or binary timestamp predates one already selected for output, the backend can
suggest a larger grace period through :cpp:member:`BackendOptions::error_notifier`.

.. code-block:: text

   Quill WARNING: A record is 571 us out of timestamp order. For delayed publishers, consider increasing BackendOptions::log_timestamp_ordering_grace_period to at least 600 us.

The suggested period adds the observed difference in whole microseconds to the current period, rounded up
to one significant digit. The worker reports only increases over its last suggestion. More time helps a
delayed producer, but cannot repair a clock running backward.

Zero grace or enabled monotonic correction disables the notification. The comparison excludes custom
``EpochNanoseconds`` timelines. Monotonic correction orders those records among themselves, separately
from real-time loggers.

Timestamp Methods
-----------------

The default frontend uses the same ``Logger*`` type for all three choices. One binary can select live or
simulation time at runtime.

- **TSC** (``ClockSource::tsc()``, the default). The caller reads raw ``__rdtsc`` ticks. The backend's
  ``RdtscClock`` takes about 10 ms to calibrate on first use, then converts ticks to wall time with a lock-free
  algorithm. It resynchronizes every 500 ms by default. Use an invariant TSC synchronized across cores.
  Drift is usually small, but captures from different cores can use different calibration points across a
  resync. Timestamps may then appear a few microseconds out of order.
- **System** (``ClockSource::system()``). ``std::chrono::system_clock::now()`` costs more on the caller but
  gives wall time immediately, without TSC calibration or resync drift. Use it when wall-time accuracy or strict
  chronological order matters.
- **User** (``ClockSource::custom(clock)``). The caller reads ``UserClockSource::now()`` virtually by default.
  The backend interprets the result using the optional virtual ``timestamp_type() const noexcept``.

Providing a Custom Timestamp
----------------------------

.. literalinclude:: ../examples/user_clock_source.cpp
   :language: cpp
   :linenos:

Frontend Clock Selection
------------------------

``FrontendOptions::clock_type`` defaults to ``quill::ClockSource``, which holds any of the three runtime choices.
Options without ``clock_type`` also use runtime selection.

To make the read eligible for inlining, set ``clock_type`` to the ``UserClockSource`` subclass that implements
``now() const noexcept``. Pass ``ClockSource::custom(clock)`` directly to the factory so it can check the type.
The option selects dispatch, and the argument supplies the instance and state.

- The selected class may be abstract if it implements ``now()`` and the instance is derived. Virtual inheritance
  from ``UserClockSource`` prevents the frontend's ``static_cast`` and the first log call fails to compile.
- Direct dispatch bypasses further overrides of ``now()``. The backend still calls ``timestamp_type()`` virtually.
  Return a ``TimestampType`` matching the selected implementation.

**Custom timestamp representations**

- ``TimestampType::TscTicks`` returns raw counter ticks. The backend converts them to epoch nanoseconds and
  applies the grace period as for the built-in TSC.
- ``TimestampType::WallClockNanoseconds`` returns real Unix-epoch nanoseconds, for example from PTP, a NIC,
  ``CLOCK_REALTIME`` or a custom TSC conversion. Ordering and grace match the system clock.
- ``TimestampType::EpochNanoseconds`` is the default for compatibility with existing clocks. Use it for
  simulations or a replay of a past trading day. Marking replay data as wall time can trigger reordering
  diagnostics or advance its timestamps to other loggers' wall times under monotonic correction.

.. literalinclude:: snippets/quill_docs_custom_wall_clock.cpp
   :language: cpp
   :start-at: // A clock that follows real time

``ClockSource::custom(clock)`` borrows the clock by reference. Keep it alive through the logger's lifetime and
pending records. Both methods must be thread-safe and the timestamp representation must stay constant.
Loggers may use different instances. Quill reads the clock once per enqueued record and skips it for dropped
records. Zero is a valid nanosecond timestamp.

Declare ``uint64_t now() const noexcept override``. It must return normally and must never log through Quill,
even via another logger. Nested logging can overwrite the caller's encoded but uncommitted record.
An exception can strand a deferred argument in it.

With a concrete clock type, the instance must have that type or derive from it. A different type, a built-in
source or a missing clock argument fails at compile time. Use consistent :doc:`frontend_options` across the
application, shared libraries and typed signal handlers.

The same factory expression works with virtual or direct dispatch.

.. literalinclude:: snippets/quill_docs_custom_clock.cpp
   :language: cpp
   :linenos:

If every logger uses TSC, this clock eliminates runtime selection. It uses the default TSC logger's inline
read and queue encoder and can save frontend instructions, depending on compiler and build options.

.. literalinclude:: snippets/quill_docs_custom_tsc_clock.cpp
   :language: cpp
   :start-at: struct TscClock

Updating Existing Logger Creation Calls
---------------------------------------

Replace the old enum/pointer pair with one clock selection. Omitting the argument still selects TSC.

.. code-block:: cpp

   // Previous: ..., pattern_options, ClockSourceType::User, &clock
   // Current:
   Frontend::create_or_get_logger("app", sink, pattern_options, ClockSource::custom(clock));

For an explicit built-in selection use ``ClockSource::tsc()`` or ``ClockSource::system()``.

Getting a Synchronized Timestamp with the Backend Thread TSC Clock
------------------------------------------------------------------

Use :cpp:class:`BackendTscClock` to obtain a timestamp synchronized with the backend's TSC log timestamps.

.. literalinclude:: snippets/quill_docs_timestamp_types.cpp
   :language: cpp
   :linenos:
