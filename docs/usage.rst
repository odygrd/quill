.. _usage:

##############################################################################
Usage
##############################################################################

Quickstart
===========

.. code:: cpp

   #include "quill/Quill.h"

   int main()
   {
     // optional configuration before calling quill::start()
     quill::Config cfg;
     cfg.enable_console_colours = true;
     quill::configure(cfg);

     // starts the logging thread
     quill::start();

     // creates a logger
     quill::Logger* logger = quill::get_logger("my_logger");

     // log
     LOG_DEBUG(logger, "Debugging foo {}", 1234);
     LOG_INFO(logger, "Welcome to Quill!");
     LOG_WARNING(logger, "A warning message.");
     LOG_ERROR(logger, "An error message. error code {}", 123);
     LOG_CRITICAL(logger, "A critical error.");
   }

Single logger object
======================

.. code:: cpp

    // When QUILL_ROOT_LOGGER_ONLY is defined then only a single root logger object is used
    #define QUILL_ROOT_LOGGER_ONLY

    #include "quill/Quill.h"

    int main()
    {
      // quill::Handler* handler = quill::stdout_handler(); /** for stdout **/
      quill::Handler* handler = quill::file_handler("quickstart.log", "w");
      handler->set_pattern("%(ascii_time) [%(thread)] %(fileline:<28) LOG_%(level_name) %(message)");

      // set configuration
      quill::Config cfg;
      cfg.default_handlers.push_back(handler);

      // Apply configuration and start the backend worker thread
      quill::configure(cfg);
      quill::start();

      LOG_INFO("Hello {}", "world");
      LOG_ERROR("This is a log error example {}", 7);
    }

Log to file
======================

.. code:: cpp

    #include "quill/Quill.h"

    int main()
    {
      quill::Handler* handler = quill::file_handler("quickstart.log", "w");
      handler->set_pattern("%(ascii_time) [%(thread)] %(fileline:<28) %(level_name) %(logger_name:<12) %(message)");

      // set configuration
      quill::Config cfg;
      cfg.default_handlers.push_back(handler);

      // Apply configuration and start the backend worker thread
      quill::configure(cfg);
      quill::start();

      auto my_logger = quill::create_logger("mylogger");
      my_logger->set_log_level(quill::LogLevel::Debug);

      LOG_INFO(my_logger, "Hello {}", "world");
      LOG_ERROR(my_logger, "This is a log error example {}", 7);
    }
