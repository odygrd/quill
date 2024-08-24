.. title:: Sink Types

Sink Types
==========

ConsoleSink
~~~~~~~~~~~

The `ConsoleSink` class sends logging output to streams `stdout` or `stderr`. Printing color codes to terminal or Windows console is also supported.

FileSink
~~~~~~~~

The :cpp:class:`quill::FileSink` is a straightforward sink that outputs to a file. The filepath of the `FileSink` serves as a unique identifier, allowing you to retrieve the same sink later using :cpp:func:`quill::FrontendImpl::get_sink`.

Each file can only have a single instance of `FileSink`.

.. code:: cpp

    int main()
    {
      // Start the backend thread
      quill::Backend::start();

      // Frontend
      auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        "trivial_logging.log",
        []()
        {
          quill::FileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
          return cfg;
        }(),
        quill::FileEventNotifier{});

      quill::Logger* logger =
        quill::Frontend::create_or_get_logger("root", std::move(file_sink));

      LOG_INFO(logger, "log something {}", 123);
    }

RotatingFileSink
~~~~~~~~~~~~~~~~

The :cpp:class:`quill::RotatingFileSink` is built on top of the `FileSink` and provides log file rotation based on specified time intervals, file sizes, or daily schedules.

.. code:: cpp

      // Start the backend thread
      quill::Backend::start();

      // Frontend
      auto rotating_file_sink = quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
        "rotating_file.log",
        []()
        {
          // See RotatingFileSinkConfig for more options
          quill::RotatingFileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
          cfg.set_rotation_time_daily("18:30");
          cfg.set_rotation_max_file_size(1024);
          return cfg;
        }());

      quill::Logger* logger =
        quill::Frontend::create_or_get_logger("root", std::move(rotating_file_sink));

      for (int i = 0; i < 20; ++i)
      {
        LOG_INFO(logger, "Hello from rotating logger, index is {}", i);
      }

JsonFileSink/JsonConsoleSink
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :cpp:class:`quill::JsonFileSink` and :cpp:class:`quill::JsonConsoleSink` enable the creation of structured logs. To utilize this feature, provide named arguments in your log format string, as shown in the example below. The example demonstrates how to create a logger that emits both normal and structured logs, but it can also be configured to emit only structured logs.

.. code:: cpp

      // Start the backend thread
      quill::Backend::start();

      // Frontend

      // Create a json file for output
      auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonFileSink>(
        "json_sink_logging.log",
        []()
        {
          quill::JsonFileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
          return cfg;
        }(),
        quill::FileEventNotifier{});

      // When using the JsonFileSink, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
      quill::Logger* json_logger = quill::Frontend::create_or_get_logger(
        "json_logger", std::move(json_sink), quill::PatternFormatterOptions { "", "%H:%M:%S.%Qns", quill::Timezone::GmtTime });

      for (int i = 0; i < 2; ++i)
      {
        LOG_INFO(json_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
      }

      // It is also possible to create a logger that logs to both the json file and stdout
      // with the appropriate format
      auto json_sink_2 = quill::Frontend::get_sink("json_sink_logging.log");
      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console_sink_id_1");

      // We set a custom format pattern here to also include the named_args
      quill::Logger* hybrid_logger = quill::Frontend::create_or_get_logger(
        "hybrid_logger", {std::move(json_sink_2), std::move(console_sink)},
        quill::PatternFormatterOptions { "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<20) "
        "%(message) [%(named_args)]" });

      for (int i = 0; i < 2; ++i)
      {
        LOG_INFO(hybrid_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
      }