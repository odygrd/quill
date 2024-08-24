.. title:: JSON Logging

JSON Logging
============

The library supports outputting JSON-structured logs to either the console or a file. To utilize this feature, you need to use named arguments within the format string, specifically inside the ``{}`` placeholders, when invoking the ``LOG_`` macros.

For convenience, the ``LOGJ_`` macros offer an alternative method for logging l-values, automatically including the argument names in the placeholders. These macros support up to 20 arguments.

It is also possible to combine JSON output with standard log pattern display by passing multiple ``Sink`` objects to a ``Logger``. This allows you to see the same log message in different formats simultaneously.

Logging Json to Console
-----------------------

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/JsonConsoleSink.h"
    #include <string>

    int main()
    {
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

      auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonConsoleSink>("json_sink_1");

      // When logging json, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
      quill::Logger* logger = quill::Frontend::create_or_get_logger(
        "json_logger", std::move(json_sink), quill::PatternFormatterOptions { "", "%H:%M:%S.%Qns", quill::Timezone::GmtTime });

      int var_a = 123;
      std::string var_b = "test";

      // Log via the convenient LOGJ_ macros
      LOGJ_INFO(logger, "A json message", var_a, var_b);

      // Or manually specify the desired names of each variable
      LOG_INFO(logger, "A json message with {var_1} and {var_2}", var_a, var_b);
    }

Logging Json to File
--------------------

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/JsonFileSink.h"
    #include <string>

    int main()
    {
      // Start the backend thread
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

      // Create a json sink
      auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonFileSink>("example_json.log",
                                                                                []()
                                                                                {
                                                                                  quill::JsonFileSinkConfig config;
                                                                                  return config;
                                                                                }());

      // When logging json, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
      quill::Logger* logger = quill::Frontend::create_or_get_logger(
        "json_logger", std::move(json_sink), quill::PatternFormatterOptions { "", "%H:%M:%S.%Qns", quill::Timezone::GmtTime });

      int var_a = 123;
      std::string var_b = "test";

      // Log via the convenient LOGJ_ macros
      LOGJ_INFO(logger, "A json message", var_a, var_b);

      // Or manually specify the desired names of each variable
      LOG_INFO(logger, "A json message with {var_1} and {var_2}", var_a, var_b);
    }

Combining JSON and Standard Log Patterns
----------------------------------------

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/ConsoleSink.h"
    #include "quill/sinks/JsonFileSink.h"
    #include <utility>

    int main()
    {
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

      // Create a json file for output
      auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonFileSink>(
        "example_json.log",
        []()
        {
          quill::JsonFileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_filename_append_option(quill::FilenameAppendOption::None);
          return cfg;
        }(),
        quill::FileEventNotifier{});

      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console_sink_id_1");

      // We set a custom format pattern here to also include the named_args
      quill::Logger* hybrid_logger = quill::Frontend::create_or_get_logger(
        "hybrid_logger", {std::move(json_sink), std::move(console_sink)},
        quill::PatternFormatterOptions { "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<20) "
        "%(message) [%(named_args)]" });

      for (int i = 2; i < 4; ++i)
      {
        LOG_INFO(hybrid_logger, "{method} to {endpoint} took {elapsed} ms", "POST", "http://", 10 * i);
      }
    }