.. _usage:

##############################################################################
Usage
##############################################################################

Quickstart
===========

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/ConsoleSink.h"

    #include <string>
    #include <utility>

    /**
     * Trivial logging example to console
     */

    int main()
    {
      // Start the backend thread
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

      // Frontend
      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
      quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

      // Change the LogLevel to print everything
      logger->set_log_level(quill::LogLevel::TraceL3);

      LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
      LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
      LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
      LOG_DEBUG(logger, "This is a log debug example {}", 4);
      LOG_INFO(logger, "This is a log info example {}", sizeof(std::string));
      LOG_WARNING(logger, "This is a log warning example {}", sizeof(std::string));
      LOG_ERROR(logger, "This is a log error example {}", sizeof(std::string));
      LOG_CRITICAL(logger, "This is a log critical example {}", sizeof(std::string));
    }


Log to file
======================

.. code:: cpp

    #include "quill/Backend.h"
    #include "quill/Frontend.h"
    #include "quill/LogMacros.h"
    #include "quill/Logger.h"
    #include "quill/sinks/FileSink.h"

    #include <utility>

    int main()
    {
      // Start the backend thread
      quill::BackendOptions backend_options;
      quill::Backend::start(backend_options);

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
        quill::Frontend::create_or_get_logger("root", std::move(file_sink),
                                              "%(time) [%(thread_id)] %(short_source_location:<28) "
                                              "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                              "%H:%M:%S.%Qns", quill::Timezone::GmtTime);

      // set the log level of the logger to debug (default is info)
      logger->set_log_level(quill::LogLevel::Debug);

      LOG_INFO(logger, "log something {}", 123);
      LOG_DEBUG(logger, "something else {}", 456);
    }