.. title:: Frontend Options

.. _frontend_options:

Frontend Options
================

The frontend options provide a flexible way to configure hot path settings at compile time. These options allow for the customisation of queue types and memory allocation strategies, including the use of huge pages on Linux systems for improved performance.

Each frontend thread operates with its own queue, which can be configured with one of the following options:

- **UnboundedBlocking**: Starts with a small initial capacity. The queue reallocates up to `FrontendOptions::unbounded_queue_max_capacity` and then blocks the calling thread until space becomes available.
- **UnboundedDropping**: Starts with a small initial capacity. The queue reallocates up to `FrontendOptions::unbounded_queue_max_capacity` and then discards log messages.
- **BoundedBlocking**: Has a fixed capacity and never reallocates. It blocks the calling thread when the limit is reached until space becomes available.
- **BoundedDropping**: Has a fixed capacity and never reallocates. It discards log messages when the limit is reached.

Even though each thread has its own queue, a single queue type must be defined for the entire application. By default the `UnboundedBlocking` queue type is used.

To modify the queue type, you should define your own :cpp:struct:`FrontendOptions` and use it to create a custom :cpp:class:`FrontendImpl` and :cpp:class:`LoggerImpl`.

It is important to consistently use your custom types throughout the application, instead of the default ones.

Error Notifications
-------------------

The library provides error notifications through the :cpp:member:`BackendOptions::error_notifier` callback:

- **Unbounded queues**: Notify when queue reallocations occur
- **Bounded queues**: Notify when messages are dropped due to full queues, including a count of dropped messages

These notifications are processed by the backend thread and can help monitor queue behavior in production.

Queue Memory Management
-----------------------

**Queue Shrinking**

For unbounded queues, you can explicitly reduce memory usage on specific threads using :cpp:func:`Frontend::shrink_thread_local_queue`. This is particularly useful in thread pool scenarios where some threads may experience logging bursts that cause queue growth, but subsequent tasks don't require the large capacity:

.. code-block:: cpp

    // After a logging-intensive task completes, shrink the queue
    quill::Frontend::shrink_thread_local_queue(512 * 1024);  // Shrink to 512KB

**Monitoring Queue Allocations**

By default, Quill automatically reports queue allocation events to stderr through :cpp:member:`BackendOptions::error_notifier`. You'll see messages like:

.. code-block:: shell

    Allocated a new SPSC queue with a capacity of 256 KiB (previously 128 KiB) from thread 3764

These allocation messages are informational, not errors, indicating the queue is dynamically resizing to handle traffic spikes. To customize this behavior, you can override the error notifier:

.. code-block:: cpp

    quill::BackendOptions backend_options{
        .error_notifier = [](const std::string& error_message) {
            // Custom handling - log to your preferred destination
            my_logger.info("Quill: {}", error_message);
        }
    };

To completely disable these notifications, set the error notifier to an empty function:

.. code-block:: cpp

    quill::BackendOptions backend_options{
        .error_notifier = {}  // Disable notifications
    };

**Mixed Hot/Cold Path Optimization**

For applications with both hot path (performance-critical) and cold path (less frequent) logging threads, you can optimize memory allocation by using a larger initial queue size globally, then selectively shrinking queues on cold path threads:

.. code-block:: cpp

    // Configure large initial capacity to avoid hot path allocations
    struct MyFrontendOptions : quill::FrontendOptions 
    {
        static constexpr size_t initial_queue_capacity = 64 * 1024 * 1024;  // 64MB
        // ... other options
    };
    
    // Hot path threads: Use the large queue (no shrinking needed)
    // LOG_INFO(hot_path_logger, "High frequency logging...");
    
    // Cold path threads: Shrink to smaller size after initialization
    void cold_path_thread_init() {
        quill::Frontend::shrink_thread_local_queue(128 * 1024);  // Shrink to 128KB
        // LOG_INFO(cold_path_logger, "Infrequent logging...");
    }

This approach prevents runtime allocations on performance-critical threads while conserving memory on threads with lighter logging workloads.

Example Usage
-------------

For example, to use a `BoundedDropping` queue with a fixed capacity of `131'072`, you can follow the steps below:

.. literalinclude:: ../examples/custom_frontend_options.cpp
   :language: cpp
   :linenos: