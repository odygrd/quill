.. usage:

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
