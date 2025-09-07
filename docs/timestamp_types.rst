.. title:: Timestamp Types

Timestamp Types
===============

Timestamp Recording
-------------------

The library records the timestamp of the log statement on the caller thread at the point the call was made.
This approach makes the log statement timestamp more accurate but introduces the possibility of the backend seeing a timestamp in the past, causing an out-of-order timestamp to appear in the log.

Due to the usage of different types of queues, it is possible in rare cases for the timestamp to be recorded but not immediately processed.
For example, the caller thread might block (if a blocking queue is used) or take some time to reallocate a new non-blocking queue.

To mitigate this, there is a configurable :cpp:member:`BackendOptions::log_timestamp_ordering_grace_period` option.
This option delays the reading of timestamps by the backend for a few microseconds (default 5μs), providing additional time for the frontend to push the messages while maintaining ordering.

Timestamp Methods
-----------------

There are several ways to get the timestamp on the frontend:

- **TSC (Time Stamp Counter)**: This is the fastest method for the frontend, as it simply calls ``__rdtsc`` and pushes the raw TSC counter value. The backend runs an ``RdtscClock`` which periodically syncs with the wall time (default every 500ms). It takes the ``rdtsc`` value from the frontend and converts it to wall time using a lock-free algorithm. \
  Although the ``rdtsc`` clock might drift between resyncs, the difference is generally negligible. However, your processor must support invariant TSC to ensure that TSC counters between different cores remain synchronized. When this timestamp type is used, the backend requires a few seconds for the initial calibration of the ``RdtscClock`` on first usage.

  **Note**: TSC timestamps in rare cases may appear out of chronological order by a few microseconds. This occurs because: (1) frontend threads on different CPU cores capture raw TSC values with potential slight variations, (2) the backend converts these TSC values to wall time using a periodically updated calibration point, and (3) when resync occurs between message captures, different messages may be converted using different calibration points. Since the RdtscClock is not monotonic across resyncs, timestamps from different cores converted with different calibrations can appear out of order even when the actual logging occurred in sequence. This is an inherent characteristic of TSC-based timing across multi-core systems. If strict chronological timestamp ordering is required, use the System clock method instead.

- **System**: This method calls ``std::chrono::system_clock::now()`` on the frontend and pushes the wall time value to the queue. It is slower than TSC but provides the most accurate and immediately usable timestamps. The backend thread does not require any additional initialization time since the ``RdtscClock`` is not used.

- **User**: This method allows the user to provide a custom timestamp in nanoseconds since epoch via ``UserClockSource``. This is useful, for example, in simulations where you want to display a custom timestamp rather than the current one in the log statements.

Providing a Custom Timestamp
----------------------------

.. literalinclude:: ../examples/user_clock_source.cpp
   :language: cpp
   :linenos:

Getting a Synchronized Timestamp with the Backend Thread TSC Clock
------------------------------------------------------------------

In some cases, when using TSC for log statements, you might want to obtain a timestamp that is synchronized with the timestamp seen in the log statements.
To achieve this, you can use the :cpp:class:`BackendTscClock`. See the example below:

.. literalinclude:: examples/quill_docs_timestamp_types_1.cpp
   :language: cpp
   :linenos:
