.. title:: Frontend Options

Frontend Options
================

The frontend options provide a flexible way to configure hot path settings. These options are compile time options and allow for the customisation of queue types, as well as the use of huge pages on Linux systems.

Each frontend thread operates with its own queue, which can be configured with one of the following options:

- **UnboundedBlocking**: Starts with a small initial capacity. The queue reallocates up to `FrontendOptions::unbounded_queue_max_capacity` and then blocks further log messages.
- **UnboundedDropping**: Starts with a small initial capacity. The queue reallocates up to `FrontendOptions::unbounded_queue_max_capacity` and then discards log messages.
- **BoundedBlocking**: Has a fixed capacity and never reallocates. It blocks log messages when the limit is reached.
- **BoundedDropping**: Has a fixed capacity and never reallocates. It discards log messages when the limit is reached.

Even though each thread has its own queue, a single queue type must be defined for the entire application. By default the `UnboundedBlocking` queue type is used.

To modify the queue type, you should define your own :cpp:struct:`FrontendOptions` and use it to create a custom :cpp:class:`FrontendImpl` and :cpp:class:`LoggerImpl`.

It is important to consistently use your custom types throughout the application, instead of the default ones.

For example, to use a `BoundedDropping` queue with a fixed capacity of `131'072`, you can follow the steps below:

.. literalinclude:: ../examples/custom_frontend_options.cpp
   :language: cpp
   :linenos: