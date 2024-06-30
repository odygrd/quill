.. title:: Filters

Filters
=======

Filters are used to selectively control which log statements are sent to specific ``Sinks`` based on defined criteria.

Each :cpp:class:`quill::Sink` can be associated with one or multiple :cpp:class:`quill::Filter` objects. These filters allow customization of log statement handling, such as filtering by log level or other criteria.

By default, a logger sends all log messages to its ``Sinks``. Filters provide a way to intercept and selectively process log records before they are outputted.

A filter is implemented as a callable object that evaluates each log statement and returns a boolean value. This boolean value determines whether the log statement should be forwarded to the ``Sink`` or filtered out.

.. code:: cpp

    struct Filter
    {
      bool operator()(quill::LogRecord const& log_record)
      {
        // return true to accept the log message, false to reject it
        return log_record.metadata.log_level() >= quill::LogLevel::Warning;
      }
    };

    int main()
    {
      // Start the backend thread
      quill::Backend::start();

      // Frontend
      auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("filtered_logging.log");

      file_sink->set_filter(Filter{});

      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));

      LOG_INFO(logger, "This log will not be written");
      LOG_WARNING(logger, "This log will be written");
    }