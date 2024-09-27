.. title:: Frontend Options

Frontend Options
================

The frontend options provide a flexible way to configure hot path settings. These options are compile time options and allow for the customisation of queue types, as well as the use of huge pages on Linux systems.

Each frontend thread operates with its own queue, which can be configured with one of the following options:

- **UnboundedBlocking**: Starts with a small initial capacity. The queue reallocates up to 2GB and then blocks further log messages.
- **UnboundedDropping**: Starts with a small initial capacity. The queue reallocates up to 2GB and then discards log messages.
- **UnboundedUnlimited**: Starts with a small initial capacity and reallocates without limit. This queue never blocks or drops log messages.
- **BoundedBlocking**: Has a fixed capacity and never reallocates. It blocks log messages when the limit is reached.
- **BoundedDropping**: Has a fixed capacity and never reallocates. It discards log messages when the limit is reached.

Even though each thread has its own queue, a single queue type must be defined for the entire application. By default the `UnboundedBlocking` queue type is used.

To modify the queue type, you should define your own :cpp:struct:`FrontendOptions` and use it to create a custom :cpp:class:`FrontendImpl` and :cpp:class:`LoggerImpl`.

It is important to consistently use your custom types throughout the application, instead of the default ones.

For example, to use a `BoundedDropping` queue with a fixed capacity of `131'072`, you can follow the steps below:

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/sinks/ConsoleSink.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include <utility>

    // define your own FrontendOptions, see "core/FrontendOptions.h" for details
    struct CustomFrontendOptions
    {
      static constexpr quill::QueueType queue_type = quill::QueueType::BoundedDropping;
      static constexpr uint32_t initial_queue_capacity = 131'072;
      static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
      static constexpr bool huge_pages_enabled = false;
    };

    // To utilize our custom FrontendOptions, we define a Frontend class using CustomFrontendOptions
    using CustomFrontend = quill::FrontendImpl<CustomFrontendOptions>;

    // The Logger type must also be defined
    using CustomLogger = quill::LoggerImpl<CustomFrontendOptions>;

    int main()
    {
      // Start the backend thread
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options); // or quill::Backend::start<CustomFrontendOptions>(backend_options, signal_handler_options);

      // All frontend operations and Logger must utilize the CustomFrontend instead of quill::Frontend
      auto console_sink = CustomFrontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
      CustomLogger* logger = CustomFrontend::create_or_get_logger("root", std::move(console_sink));

      // log something
      LOG_INFO(logger, "This is a log info example {}", 123);
      LOG_WARNING(logger, "This is a log warning example {}", 123);
    }