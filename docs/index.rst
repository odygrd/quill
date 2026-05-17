.. meta::
   :google-site-verification: OdGHhtE4NLXZfqvQWdVxnV4z8MJeRUws09jAYCDjqhI

.. _index:

Fast asynchronous logging for low-latency C++ applications.

Quick Example
-------------

.. code-block:: cpp

   #include "quill/LogMacros.h"
   #include "quill/SimpleSetup.h"

   int main()
   {
     auto* logger = quill::simple_logger();
     LOG_INFO(logger, "Hello from {}!", "Quill");
   }

A macro-free interface (``quill::info()``, ``quill::warning()``, ...) is also available — see :doc:`Macro-Free Mode <macro_free_mode>`.

Use :doc:`Quick Start <quick_start>` for the smallest setup, or move to the full
``Backend`` / ``Frontend`` APIs when you need custom sinks, multiple loggers, or more
explicit lifecycle control.

Start Here
----------

- :doc:`Get Started <quick_start>` for the shortest path to working logs
- :doc:`Installing <installing>` for package manager and source setup
- :doc:`Guides <guides>` for sinks, formatters, JSON, filters, and more
- :doc:`Recipes <recipes>` for common tasks and examples
- :doc:`FAQ <faq>` for integration guidance and common pitfalls

.. toctree::
   :maxdepth: 2
   :caption: Home
   :hidden:

   self

.. toctree::
   :maxdepth: 2
   :caption: Get Started
   :hidden:

   quick_start
   installing
   basic_example
   overview
   faq

.. toctree::
   :maxdepth: 2
   :caption: Guides
   :hidden:

   guides

.. toctree::
   :maxdepth: 2
   :caption: Recipes
   :hidden:

   recipes

.. toctree::
   :maxdepth: 2
   :caption: API
   :hidden:

   users-api
