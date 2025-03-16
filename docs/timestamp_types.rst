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
This option delays the reading of timestamps by the backend for a few milliseconds, providing additional time for the frontend to push the messages.

Timestamp Methods
-----------------

There are several ways to get the timestamp on the frontend:

- **TSC (Time Stamp Counter)**: This is the fastest method for the frontend, as it simply calls ``__rdtsc`` and pushes the value of the TSC counter. The backend runs an ``RdtscClock`` which periodically syncs with the wall time. It takes the ``rdtsc`` value from the frontend and converts it to wall time. \
  Although the ``rdtsc`` clock might drift between resyncs, the difference is generally negligible. However, since the backend thread might be running on a separate core, your processor should support invariant TSC to ensure that the TSC counters between different processors are in sync. The OS usually syncs the counters on startup. When this type of timestamp is used the backend also requires a few seconds for the initial sync of the ``RdtscClock``, which happens only once on the first usage of the clock.

- **System**: This method calls ``std::chrono::system_clock::now()`` on the frontend and pushes the value to the queue. It is slower but the most accurate method. The backend thread does not require any additional time for initialising the internal ``RdtscClock`` clock as is not used.

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
