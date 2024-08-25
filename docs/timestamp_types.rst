.. title:: Timestamp Types

Timestamp Types
===============

Timestamp Recording
-------------------

The library records the timestamp of the log statement on the caller thread at the point the call was made.
This approach makes the log statement timestamp more accurate but introduces the possibility of the backend seeing a timestamp in the past, causing an out-of-order timestamp to appear in the log.

Due to the usage of different types of queues, it is possible in rare cases for the timestamp to be recorded but not immediately processed.
For example, the caller thread might block (if a blocking queue is used) or take some time to reallocate a new non-blocking queue.

To mitigate this, there is a configurable :cpp:member:`quill::BackendOptions::log_timestamp_ordering_grace_period` option.
This option delays the reading of timestamps by the backend for a few milliseconds, providing additional time for the frontend to push the messages.

Timestamp Methods
-----------------

There are several ways to get the timestamp on the frontend:

- **TSC (Time Stamp Counter)**: This is the fastest method for the frontend, as it simply calls ``__rdtsc`` and pushes the value of the TSC counter. The backend runs an ``RdtscClock`` which periodically syncs with the wall time. It takes the ``rdtsc`` value from the frontend and converts it to wall time. Although the ``rdtsc`` clock might drift between resyncs, the difference is generally negligible. However, since the backend thread might be running on a separate core, your processor should support invariant TSC to ensure that the TSC counters between different processors are in sync. The OS usually syncs the counters on startup. When this type of timestamp is used the backend also requires a few seconds for the initial sync of the ``RdtscClock``, which happens only once on the first usage of the clock.

- **System**: This method calls ``std::chrono::system_clock::now()`` on the frontend and pushes the value to the queue. It is slower but the most accurate method. The backend thread does not require any additional time for initialising the internal ``RdtscClock`` clock as is not used.

- **User**: This method allows the user to provide a custom timestamp in nanoseconds since epoch via ``UserClockSource``. This is useful, for example, in simulations where you want to display a custom timestamp rather than the current one in the log statements.

Providing a Custom Timestamp
----------------------------

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/UserClockSource.h"
    #include "quill/sinks/ConsoleSink.h"

    #include <atomic>
    #include <chrono>
    #include <cstdint>
    #include <utility>

    class SimulatedClock : public quill::UserClockSource
    {
    public:
      SimulatedClock() = default;

      /**
       * Required by TimestampClock
       * @return current time now, in nanoseconds since epoch
       */
      uint64_t now() const override { return _timestamp_ns.load(std::memory_order_relaxed); }

      /**
       * set custom timestamp
       * @param time_since_epoch timestamp
       */
      void set_timestamp(std::chrono::seconds time_since_epoch)
      {
        // always convert to nanos
        _timestamp_ns.store(static_cast<uint64_t>(std::chrono::nanoseconds{time_since_epoch}.count()),
                            std::memory_order_relaxed);
      }

    private:
      std::atomic<uint64_t> _timestamp_ns{0}; // time since epoch - must always be in nanoseconds
    };

    int main()
    {
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

      // Create a simulated timestamp class. Quill takes a pointer to this class,
      // and the user is responsible for its lifetime.
      // Ensure that the instance of this class remains alive for as long as the logger
      // object exists, until the logger is removed.
      SimulatedClock simulated_clock;

      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
      quill::Logger* logger = quill::Frontend::create_or_get_logger(
        "root", std::move(console_sink),
        quill::PatternFormatterOptions { "%(time) %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) %(message)",
        "%D %H:%M:%S.%Qns", quill::Timezone::LocalTime }, quill::ClockSourceType::User, &simulated_clock);

      // Set our timestamp to Sunday 12 June 2022
      simulated_clock.set_timestamp(std::chrono::seconds{1655007309});
      LOG_INFO(logger, "This is a log trace l3 example {}", 1);

      // update our timestamp
      simulated_clock.set_timestamp(std::chrono::seconds{1655039000});
      LOG_INFO(logger, "This is a log info {} example", "string");
    }

Getting a Synchronized Timestamp with the Backend Thread TSC Clock
------------------------------------------------------------------

In some cases, when using TSC for log statements, you might want to obtain a timestamp that is synchronized with the timestamp seen in the log statements.
To achieve this, you can use the :cpp:class:`quill::BackendTscClock`. See the example below:

.. code-block:: cpp

    #include "quill/Backend.h"
    #include "quill/BackendTscClock.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/ConsoleSink.h"

    #include <iostream>
    #include <utility>

    int main()
    {
      quill::Backend::start();

      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

      // Ensure at least one logger with quill::ClockSourceType::Tsc is created for BackendTscClock to function
      quill::Logger* logger = quill::Frontend::create_or_get_logger(
        "root", std::move(console_sink),
        quill::PatternFormatterOptions { "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) "
        "%(message)",
        "%H:%M:%S.%Qns", quill::Timezone::LocalTime }, quill::ClockSourceType::Tsc);

      // Log an informational message which will also init the backend RdtscClock
      LOG_INFO(logger, "This is a log info example with number {}", 123);

      // The function `quill::detail::BackendManager::instance().convert_rdtsc_to_epoch_time(quill::detail::rdtsc())`
      // will return a valid timestamp only after the backend worker has started and processed
      // at least one log with `ClockSourceType::Tsc`.
      // This is because the Rdtsc clock is lazily initialized by the backend worker on the first log message.
      // To ensure at least one log message is processed, we call flush_log here.
      logger->flush_log();

      // Get a timestamp synchronized with the backend's clock
      uint64_t const backend_timestamp = quill::BackendTscClock::now().time_since_epoch().count();
      std::cout << "Synchronized timestamp with the backend: " << backend_timestamp << std::endl;

      return 0;
    }